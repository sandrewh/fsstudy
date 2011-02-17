#include <string.h>
#include <ctype.h>
#include <time.h>

#include "fentry.h"
#include <stdio.h> //printf
#include <stdlib.h> //exit

void
f_entry_set_attr (f_entry *file, uint8_t attr)
{
	file->attr_r = (attr & 0x01);
	file->attr_h = (attr & 0x02);
	file->attr_s = (attr & 0x04);
	file->attr_v = (attr & 0x08);
	file->attr_d = (attr & 0x10);
	file->attr_a = (attr & 0x20);
}

uint8_t
f_entry_get_attr (f_entry *file)
{
	return file->attr_r | file->attr_h | file->attr_s | file->attr_v | file->attr_d | file->attr_a;
}

void
datetime_decode (DateTime* dt, uint8_t d1, uint8_t d2, uint8_t t1, uint8_t t2)
{
	dt->h = t2 >> 3;
	dt->min = (t1 >> 5) | ((t2 & 0x07) << 3);
	dt->s = (t1 & 0x1f) << 1;

	dt->y = 1980 + (d2 >> 1);
	dt->mon = (d1 >> 5) | ((d2 & 0x01) << 5);
	dt->d = d1 & 0x1f;
}

void
f_entry_set_datetimes (f_entry *file, uint8_t* entry)
{
	//todo: add created time-fine res. 10 ms units [0, 199] stored in 0x0d (1 byte)
	datetime_decode(&file->created, entry[0x10], entry[0x11], entry[0x0e], entry[0x0f]);
	datetime_decode(&file->accessed, entry[0x12], entry[0x13], 0, 0);
	datetime_decode(&file->modified, entry[0x18], entry[0x19], entry[0x16], entry[0x17]);
}

void
from_utf16 (uint8_t* buffer, int len)
{
	int i;
	for (i=0;i<len;i++)
	{
		buffer[i] = buffer[i*2];
	//	printf("%c = %#x %#x\n", buffer[i], buffer[i*2], buffer[i*2+1]);

		if (buffer[i] == 0xff) buffer[i] = 0;
	}
}

void
to_utf16 (char* buffer, int len, int cap)
{
	int i;
	for (i=len-1;i>=0;i--)
	{
		buffer[i*2] = buffer[i];
		buffer[i*2+1] = 0;
	}
	
	for (i=len*2;i<cap;i++)
	{
		buffer[i] = -1;
	}
}

void
f_entry_update_lfn (f_entry *file, uint8_t* entry)
{
	/* first lfn in series? */
	if (entry[0] & 0x40) file->lfn[0] = 0;
	
	char lfn_piece[128];
	lfn_piece[0] = 0;

	//strip out UTF-16
	from_utf16(entry+0x01, 5);
	strncat(lfn_piece, (char*)entry+1, 5);
	from_utf16(entry+0x0e, 6);
	strncat(lfn_piece, (char*)entry+0x0e, 6);
	from_utf16(entry+0x1c, 2);
	strncat(lfn_piece, (char*)entry+0x1c, 2);

	strcat(lfn_piece, file->lfn);
	strcpy(file->lfn, lfn_piece);	
}

void
fentry_set_unique_name_from_lfn (f_entry *f)
{
	//TODO: generate meaningful names
	// see: http://en.wikipedia.org/wiki/8.3_filename
	memset(f->name, ' ', 11);
	srand(time(0));
	
	int name_len = 8;
	if (f->attr_d) name_len = 11;
	
	int i;
	for(i=0;i<name_len;i++)
	{
		unsigned int c = rand() % 256;
		if ((c >= '0' && c <= '9')
			|| (c >= 'A' && c <= 'Z')
			|| (c == '_')
			|| (c == '~'))
		{
			f->name[i] = c;
		} else {
			i--;
		}
	}
	
	if (!f->attr_d)	
	{
		char *ext = strrchr(f->lfn, '.');
		if (ext)
		{
			for (i=8;i<12;i++)
			{
				if (!*ext) break;
				f->name[i] = *ext++;
			}
		}		
	}
}

void
fentry_convert_name_to_8_3 (f_entry *f)
{
	if (f->attr_d)
	{
		int i;
		int before_end = 1;
		for (i=0;i<11;i++)
		{
			if (!f->name[i]) before_end = 0;
			
			if (before_end)
			{
				f->name[i] = toupper(f->name[i]);
			} else {
				f->name[i] = ' ';
			}
		}
		
		f->name[11] = 0;		
		return;
	}
	
	int ext_len = 0;
	char *period = strrchr(f->lfn, '.');
	if (period) ext_len = strlen(period+1);
	if (ext_len > 3) ext_len = 3;
	if (ext_len)
	{
		/* move ext */
		memmove(f->name+8, period+1, ext_len);		
	}
	char* i;
	/* spaces afer ext */
	for (i=f->name+8+ext_len;i<f->name+11;i++)
		*i=' ';		

	if (period)
	{
		/* spaces b/w name and ext */
		for (i=period;i<f->name+8;i++)
			*i=' ';		
	}
		
	/* uppercase all chars */
	for (i=f->name;i<f->name+11;i++)
		*i=toupper(*i);

	f->name[11] = 0;
}

/* courtesy http://en.wikipedia.org/wiki/File_Allocation_Table */
uint8_t
lfn_checksum (const uint8_t *pFcbName)
{
	int i;
	uint8_t sum=0;
 
	for (i=11; i; i--)
		sum = ((sum & 1) << 7) + (sum >> 1) + *pFcbName++;
	return sum;
}

void
f_entry_set_name_from_8_3 (f_entry *file)
{
	int j, i = 0;
	/* make the file lowercase */
	//while ((file->name[i] = tolower(file->name[i++])));

	if (!file->attr_d && file->name[8] != ' ') /* not a directory - has ext */
	{
		/* find first space or end of file (except 3 char ext) */
		for (i=0;i<8;i++) if (file->name[i]==' ') break;
		/* name+i = space b/w name and ext or first char of ext */
		if (file->name[i]==' ')
		{
			file->name[i++] = '.';
			for (j = 8;j<12;j++) file->name[i++] = file->name[j];
		} else {
			memcpy(file->name+i+1, file->name+i, 4);
			file->name[8] = '.';
		}
	}

	/* truncate trailing spaces */
	for (i=0;i<12;i++)
	{
		if (file->name[i] == ' ')
		{
			file->name[i] = 0;
			break;
		}
	}
}

#if 0
f_entry*
fentry_from_subpath (part_info *p, f_entry *d, uint8_t* path)
{
	path = strdup(path);
	uint8_t *search_name = strtok(path, "/");	
	f_entry *child = 0, *parent = search_dir(p, d->first_cluster, search_name);	

	for (;;)
	{
		search_name = strtok(0, "/");
		if (!search_name) 
		{
//			printf("found %s\n", parent->name);
			free(path);
			return parent;			
		}

		if (!parent->attr_d) 
		{
			free(path);
			return 0;
		}

//		printf("looking for %s in %s\n", search_name, parent->name);
		child = search_dir(p, parent->first_cluster, search_name);

		if (!child)
		{
			free(path);
			return 0;
		}

		free(parent);
		parent = child;
	}	
}
#endif


