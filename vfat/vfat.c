#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct {
	char oem[9], label[12];
	unsigned short bytes_per_sector;
	unsigned char sectors_per_cluster;
	unsigned short reserved_sectors;
	unsigned char num_fats;
	unsigned short max_root_entries;
	unsigned int total_sectors;
	unsigned char media;
	unsigned int sectors_per_fat;
	unsigned short fs_info_sector;
	unsigned int first_sector;

	unsigned int fat_first_sector;
	unsigned int clusters_first_sector;
	unsigned int root_first_cluster;
	unsigned int free_clusters;
	unsigned int bytes_per_cluster;
	
	FILE* f;
} part_info;

typedef struct {
	unsigned char h, min, s;
	unsigned char mon, d;
	unsigned int y;
} DateTime;

typedef struct {
	unsigned int first_cluster;
	unsigned int size_bytes;
	char name[13];
	char lfn[128];
	
	unsigned char attr_r, attr_h, attr_s, attr_v, attr_d, attr_a;

	DateTime created, accessed, modified;
} f_entry;

void f_entry_set_attr(f_entry *file, unsigned char attr)
{
	file->attr_r = (attr & 0x01);
	file->attr_h = (attr & 0x02);
	file->attr_s = (attr & 0x04);
	file->attr_v = (attr & 0x08);
	file->attr_d = (attr & 0x10);
	file->attr_a = (attr & 0x20);
}

void datetime_decode(DateTime* dt, unsigned char d1, unsigned char d2, unsigned char t1, unsigned char t2)
{
	dt->h = t2 >> 3;
	dt->min = (t1 >> 5) | ((t2 & 0x07) << 3);
	dt->s = (t1 & 0x1f) << 1;

	dt->y = 1980 + (d2 >> 1);
	dt->mon = (d1 >> 5) | ((d2 & 0x01) << 5);
	dt->d = d1 & 0x1f;
}

void f_entry_set_datetimes(f_entry *file, char* entry)
{
	//todo: add created time, fine res. 10 ms units [0, 199] stored in 0x0d (1 byte)
	datetime_decode(&file->created, entry[0x10], entry[0x11], entry[0x0e], entry[0x0f]);
	datetime_decode(&file->accessed, entry[0x12], entry[0x13], 0, 0);
	datetime_decode(&file->modified, entry[0x18], entry[0x19], entry[0x16], entry[0x17]);
}

void strip_utf16(char* buffer, int len)
{
	int i;
	for (i=0;i<len;i++)
	{
		buffer[i] = buffer[i*2];
		if (buffer[i] == -1) buffer[i] = 0;
	}
}

void f_entry_update_lfn(f_entry *file, char* entry)
{
	// TODO: check lfn checksum, see: http://en.wikipedia.org/wiki/File_Allocation_Table
	
	/* firsy lfn in series? */
	if (entry[0] & 0x40) file->lfn[0] = 0;
	
	char lfn_piece[128];
	lfn_piece[0] = 0;

	//strip out UTF-16
	strip_utf16(entry+0x01, 5);
	strncat(lfn_piece, entry+1, 5);
	strip_utf16(entry+0x0e, 6);
	strncat(lfn_piece, entry+0x0e, 6);
	strip_utf16(entry+0x1c, 2);
	strncat(lfn_piece, entry+0x1c, 2);

	strcat(lfn_piece, file->lfn);
	strcpy(file->lfn, lfn_piece);	
}

#if 0
void print_tabs(int n) {
	int i;
	for (i = 0; i < n; ++i) {
		printf("\t");
	}
}
#endif

void write_cluster(part_info *p, char* buffer, unsigned int cluster)
{
	unsigned int first_sector = p->first_sector + p->clusters_first_sector + (cluster - 2) * p->sectors_per_cluster;

	fseek(p->f, first_sector * p->bytes_per_sector, SEEK_SET);
	fwrite(buffer, 1, p->bytes_per_cluster, p->f);	
}

void read_cluster(part_info *p, char* buffer, unsigned int cluster)
{
	unsigned int first_sector =  p->first_sector + p->clusters_first_sector + (cluster - 2) * p->sectors_per_cluster;

	fseek(p->f, first_sector * p->bytes_per_sector, SEEK_SET);	
	fread(buffer, 1, p->bytes_per_cluster, p->f);
}

void read_sector(part_info *p, char* buffer, unsigned int sector)
{
	fseek(p->f, (p->first_sector + sector) * p->bytes_per_sector, SEEK_SET);	
	fread(buffer, 1, p->bytes_per_sector, p->f);
}

unsigned int read_fat(part_info *p, unsigned int cluster)
{
	/* only use 1st fat, for now */
	// TODO: why use other fats? */
	
	unsigned int ret;
	fseek(p->f, (p->first_sector + p->fat_first_sector) * p->bytes_per_sector + cluster * 4, SEEK_SET);	
	fread(&ret, 1, 4, p->f);
	ret &= 0xFFFFFFF;
	return ret;
}

void write_fat(part_info *p, unsigned int cluster, unsigned int value)
{
	/* write entry to all fats */
	
	value &= 0xFFFFFFF; /* neccessary? */

	unsigned int fat_num;
	for (fat_num=0;fat_num<p->num_fats;fat_num++)
	{
		fseek(p->f, (p->first_sector + p->fat_first_sector + fat_num * p->sectors_per_fat) * p->bytes_per_sector + cluster * 4, SEEK_SET);	
		fwrite(&value, 1, 4, p->f);
	}	
}

unsigned int find_free_cluster(part_info *p)
{
	/* search through fat until free cluster is found */
	unsigned int i;
	for (i=2;i<p->total_sectors/p->sectors_per_cluster;i++)
	{
		if (!read_fat(p, i)) return i;
	}
	
	return 0; /* disk full */
}

void f_entry_convert_name(f_entry *file)
{
	int j, i = 0;
	/* make the file lowercase */
	while ((file->name[i] = tolower(file->name[i++])));

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

void print_human_size(float size)
{
	if (size < 1024.0)
	{
		printf("%6.1fB", size);
		return;
	}
	size/=1024.0;
	if (size < 1024.0)
	{
		printf("%6.1fK", size);
		return;
	}
	size/=1024.0;
	if (size < 1024.0)
	{
		printf("%6.1fM", size);
		return;
	}
	size/=1024.0;
	if (size < 1024.0)
	{
		printf("%6.1fG", size);
		return;		
	}
	size/=1024.0;
	printf("%6.1fT", size);
}

f_entry* enumerate_dir(part_info* p, unsigned int cur_cluster, int callback(f_entry f, void *arg), void *arg)
{
	/* TODO: what if entries_per_cluster is not a whole number? */
	/* that is, what if p->bytes_per_cluseter % file_entry_size != 0 */
	
	const int file_entry_size = 0x20;
	const unsigned int entries_per_cluster = p->bytes_per_cluster / 32;
	char cluster_buffer[p->bytes_per_cluster];

	// read the first cluster of the directory file
	read_cluster(p, cluster_buffer, cur_cluster);

	f_entry file;
	file.lfn[0] = 0;

	int entry_num = 0;
	// iterate over all the directory entries
	for (;;) {
		//if we reached end of cluster, load the next, if it exists
		if (entry_num >= entries_per_cluster) {
			//seek to next cluster of dir file
			cur_cluster = read_fat(p, cur_cluster);
			if (cur_cluster >= 0xFFFFFF8) break; /* end of file */
			read_cluster(p, cluster_buffer, cur_cluster);
			entry_num = 0;
		}

		char *cur_entry = cluster_buffer + entry_num*file_entry_size;
		entry_num++;
		memcpy(file.name, cur_entry, 11);
		file.name[11] = file.name[12] = 0;

		if (!file.name[0]) break; // end of listing
		if ((unsigned char)file.name[0] == 0xe5) continue; // marked unused
		// translate 0x05 to value 0xe5
		file.name[0] = (file.name[0] == 0x05 ? 0xe5 : file.name[0]);

		f_entry_set_attr(&file, cur_entry[11]);
		f_entry_set_datetimes(&file, cur_entry);

		// LFN entry?
		if (file.attr_r && file.attr_h && file.attr_s && file.attr_v) {
			if (cur_entry[0] & 0x80) continue; // deleted
			f_entry_update_lfn(&file, cur_entry);
			continue;
		}

		// Volume name?
		if (file.attr_v) continue;		
		
		unsigned short cluster_high, cluster_low;
		cluster_high = *(unsigned short*)(cur_entry+0x14);
		cluster_low = *(unsigned short*)(cur_entry+0x1a);
		file.size_bytes = *(unsigned int*)(cur_entry+0x1c);
		file.first_cluster = (((unsigned int)cluster_high) << 16) | ((unsigned int)cluster_low);
		f_entry_convert_name(&file);

		if (!callback(file, arg))
		{
			f_entry *f = (f_entry*)malloc(sizeof(f_entry));
			return memcpy(f, &file, sizeof(f_entry));
		}

		file.lfn[0] = 0;
	} /* for(;;) */

	return 0;
}

int search_dir_callback(f_entry file, void *arg)
{
	char* search_name = (char*)arg;
	char* match_name = 0;
	// does current file have a 8.3 name that matches the search?
	if (strcasecmp(search_name, file.name) == 0) match_name = file.name;
	// does current file have an lfn that matches the search?
	if (file.lfn[0] && strcasecmp(search_name, file.lfn) == 0) match_name = file.lfn;

	if (match_name)
		return 0;

	return 1;
}

f_entry* search_dir(part_info *p, unsigned int dir_cluster, char* search_name)
{
	return enumerate_dir(p, dir_cluster, search_dir_callback, (void*)search_name);
}

#if 0
f_entry* fentry_from_subpath(part_info *p, f_entry *d, char* path)
{
	path = strdup(path);
	char *search_name = strtok(path, "/");	
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

f_entry* fentry_from_path(part_info *p, char *path)
{
	path = strdup(path);
	char *search_name = strtok(path, "/");
	f_entry *parent, *child = 0;
	
	if (!search_name)
	{
		/* return root */
		parent = malloc(sizeof(f_entry));
		
		parent->first_cluster = p->root_first_cluster;
		parent->size_bytes = 0;
		
		parent->name[0] = parent->lfn[0] = '/';
		parent->name[1] = parent->lfn[1] = 0;

		char blank_datetime[0x20];
		bzero(blank_datetime, 0x20);

		parent->attr_r = 0x01;
		parent->attr_h = 0;
		parent->attr_s = 0x04;
		parent->attr_v = 0;
		parent->attr_d = 0x10;
		parent->attr_a = 0;
				
		f_entry_set_datetimes(parent, blank_datetime);

		free(path);
		return parent;
	}

	parent = search_dir(p, p->root_first_cluster, search_name);	

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

int read_file(part_info *p, f_entry *f, char* buffer, size_t size, off_t offset)
{	
	/* does path exists? */
	if (!f) return 0;
	/* is it a FILE? */
	if (f->attr_d) return 0;
	/* are we reading past end of file? */
	if (offset >= f->size_bytes) return 0;
	
	/* don't read past EOF */
	if (offset + size > f->size_bytes)
		size = f->size_bytes - offset;
		
	unsigned int cur_cluster = f->first_cluster;		
	unsigned int offset_cluster_num = offset / p->bytes_per_cluster;

	/* skip to the cluster with our offset */
	int cluster_num;
	for (cluster_num=0;cluster_num<offset_cluster_num;cluster_num++)
		cur_cluster = read_fat(p, cur_cluster);

	char cluster_buffer[p->bytes_per_cluster];
	unsigned int cluster_byte_offset = offset % p->bytes_per_cluster;
//	printf("starting at byte %d of cluster %d (%d)\n\n", cluster_byte_offset, offset_cluster_num, cur_cluster);

	unsigned int bytes_read = 0;
	while (bytes_read < size)
	{
		read_cluster(p, cluster_buffer, cur_cluster);
		
		unsigned int bytes_to_read = p->bytes_per_cluster - cluster_byte_offset;
		if (size - bytes_read < bytes_to_read) bytes_to_read = size - bytes_read;
		
//		printf("%d bytes from cluster %d::%d to buffer[%d]\n", bytes_to_read, cur_cluster, cluster_byte_offset, bytes_read);
		
		memcpy(buffer+bytes_read, cluster_buffer+cluster_byte_offset, bytes_to_read);
		cluster_byte_offset = 0;
		cur_cluster = read_fat(p, cur_cluster);
		bytes_read += bytes_to_read;
	}

	return size;
}

typedef struct {
	unsigned entry_num;
	unsigned offset;
	unsigned size;
	unsigned int num_read;
	f_entry *entries;
} search_dir_data;

int read_dir_callback(f_entry file, void *arg)
{
	search_dir_data *data = arg;
	
	if (data->entry_num++ < data->offset) return 1; /* keep going */
	if (data->num_read >= data->size) return 0; /* stop */

	data->entries[data->num_read++] = file;
	
	return 1; /* keep going */
}

unsigned int read_dir(part_info *p, char* path, f_entry *entries, size_t size, off_t offset)
{	
	f_entry *f = fentry_from_path(p, path);
	if (!f)
	{
		printf("read_dir: directory %s does not exist\n", path);
		return 0;
	}
		
	if (!f->attr_d)
	{
		printf("read_dir: path %s is not a directory\n", path);
		return 0;		
	}

	search_dir_data data;
	data.entry_num = 0;
	data.offset = offset;
	data.size = size;
	data.num_read = 0;
	data.entries = (f_entry*)malloc(size*sizeof(f_entry));

	enumerate_dir(p, f->first_cluster, read_dir_callback, (void*)&data);

	memcpy(entries, data.entries, data.num_read * sizeof(f_entry));
	free(data.entries);

	return data.num_read;
}

void cmd_ls(part_info *p, char *path)
{
	f_entry *d = fentry_from_path(p, path);
	if (!d)
	{
		printf("Directory %s not found.\n", path);
		return;
	}
	
	if (!d->attr_d)
	{
		printf("%s is not a directory.\n", path);
		free(d);
		return;
	}
	
	printf("Directory listing of %s (%x)\n", path, d->first_cluster);
	free(d);

	f_entry entries[10];
	unsigned int total_files_read = 0;
	unsigned int files_read = read_dir(p, path, entries, 10, 0);
	
	while (files_read)
	{
		total_files_read += files_read;
		
		unsigned int i;
		for (i=0;i<files_read;i++)
		{
			if (entries[i].attr_r) printf("r");
				else printf("-");
			if (entries[i].attr_d) printf("d");
				else printf("-");
			if (entries[i].attr_s) printf("s");
				else printf("-");
			if (entries[i].attr_a) printf("a");
				else printf("-");
			if (entries[i].attr_h) printf("h");
				else printf("-");				

			printf(" ");
			print_human_size(entries[i].size_bytes);
			
			if (entries[i].created.mon)
				printf(" %02d-%02d-%04d %02d:%02d ", entries[i].created.mon, entries[i].created.d, entries[i].created.y, entries[i].created.h, entries[i].created.min);
			else
				printf(" -                ");
			
			printf("%6x  ", entries[i].first_cluster);
			
			if (entries[i].lfn[0])
			{
				printf("%s\n", entries[i].lfn);	
			} else {
				printf("%s\n", entries[i].name);				
			}
		}
		
		files_read = read_dir(p, path, entries, 10, total_files_read);
	}
	
	printf("%d entries.\n", total_files_read);
}

int dir_rem_entry(part_info *p, f_entry *d, f_entry *f)
{
	return 0;
}

int dir_add_entry(part_info *p, f_entry *d, f_entry* f)
{
	/* TODO: handle lfn */
	if (f->lfn[0]) 
	{
		printf("dir_add_entry: no LFN support\n");
		return 0; /* no LFN for now */
	}
	
	const int file_entry_size = 0x20;
	const unsigned int entries_per_cluster = p->bytes_per_cluster / 32;
	char cluster_buffer[p->bytes_per_cluster];
	unsigned int cur_cluster = d->first_cluster;
	
	// read the first cluster of the directory file
	read_cluster(p, cluster_buffer, cur_cluster);

	int entry_num = 0;
	// iterate over all the directory entries
	for (;;) {
		//if we reached end of cluster, load the next, if it exists
		if (entry_num >= entries_per_cluster) {
			//seek to next cluster of dir file
			cur_cluster = read_fat(p, cur_cluster);
			if (cur_cluster >= 0xFFFFFF8)
			{
				/* end of file */	
				/* TODO: need to locate and append a free cluster */
				break; /* just give up for now, though */
			}
			read_cluster(p, cluster_buffer, cur_cluster);
			entry_num = 0;
		}

		char *cur_entry = cluster_buffer + entry_num*file_entry_size;
		entry_num++;

		if (!cur_entry[0]) 
		{
			printf("dir_add_entry: the 'end of directory' entry was found. todo: insert new entry here and move eod down.\n");
			break; // end of listing	/* TODO: should be able to put new entry here */
		}

		// if marked unused -- success - not for LFN, though - need more slots for that
		if ((unsigned char)cur_entry[0] == 0xe5)
		{
			/* TODO: convert 8.3 name to space-padded */
			strcpy(cur_entry, f->name);

			// translate 0xe5 to value 0x05
			cur_entry[0] = ((unsigned char)cur_entry[0] == 0xe5 ? 0x05 : cur_entry[0]);
			// encode f_entry			
			cur_entry[0x0b] = f->attr_r | f->attr_h | f->attr_s | f->attr_v | f->attr_d | f->attr_a;
			// TODO: encode times
			memset(cur_entry+0x0c, 0, 0x14);
			
			unsigned int new_cluster = find_free_cluster(p);
			if (!new_cluster)
			{
				printf("dir_add_entry: no free cluster found");
				return 0;
			}
			/* mark the new cluster as used (last in chain) */
			write_fat(p, new_cluster, 0xFFFFFFF);
			
			/* msb */
			cur_entry[0x14] = (new_cluster >> 16) & 0xff; /* lsb */
			cur_entry[0x15] = new_cluster >> 24; /* msb */

			/* lsb */
			cur_entry[0x1a] = new_cluster & 0xff; /* lsb */
			cur_entry[0x1b] = (new_cluster >> 8) & 0xff; /* msb */
			
			int size = 0;
			memcpy(&cur_entry[0x1c], &size, 4);
			
			// write the updated cluster to file
			write_cluster(p, cluster_buffer, cur_cluster);
			
			// empty the new cluster for the subdir
			cluster_buffer[0] = 0;
			write_cluster(p, cluster_buffer, new_cluster);
			
			return 1;
		}
	}
	
	return 0;
}

int file_rem(part_info *p, f_entry *f)
{
	return 0;
}

int entry_create(part_info *p, f_entry *d, char *name, int attr_d)
{
	f_entry f;
	
	f.first_cluster = 0; /* empty file */
	f.size_bytes = 0;
	
	/* TODO: handle LFN */
	/* different way for directories */
	/* if name is lfn, downconvert to a unique 8.3 */
	/* else use 8.3 name only */
	if (strlen(name) > 11 + attr_d) return 0; /* no LFN for now */
	strcpy(f.name, name);
	f.lfn[0] = 0;

	/* TODO: use realtime datetimes */
	char blank_datetime[0x20];
	bzero(blank_datetime, 0x20);

	f.attr_r = 0;
	f.attr_h = 0;
	f.attr_s = 0;
	f.attr_v = 0;
	f.attr_d = attr_d;
	f.attr_a = 0;
			
	f_entry_set_datetimes(&f, blank_datetime);
	
//	printf("attr_d: %d\n", f.attr_d); exit(1);

	return dir_add_entry(p, d, &f);
}

int dir_create(part_info *p, f_entry *d, char* name)
{
	printf("creating directory '%s' in '%s'\n", name, d->name);
	return entry_create(p, d, name, 0x10); /* 0x10 = subdir */
}

int file_create(part_info *p, f_entry *d, char* name)
{
	return entry_create(p, d, name, 0);	
}

int cmd_mkdir(part_info *p, char* dir_path)
{
	f_entry *d = fentry_from_path(p, dir_path);
	if (d)
	{
		printf("directory %s already exists.\n", dir_path);
		free(d);
		return 0;
	}

	/* get the f_entry of the path leading up to the last '/' */
	char* last_slash = strrchr(dir_path, '/');
	/* everything after last slash is _name_ */
	char* name = strdup(last_slash+1);
	last_slash[0] = 0; /* termiante path at last slash */
	d = fentry_from_path(p, dir_path);
	int ret = dir_create(p, d, name);
	free(name);
	free(d);
	return ret;
}

int cmd_mv(part_info *p, char* src_path, char* dst_path)
{
	printf("moving %s => %s\n", src_path, dst_path);
	f_entry *f = fentry_from_path(p, dst_path);
	if (f)
	{
		printf("destination: %s, arlready exists\n", dst_path);
		free(f);
		return 0;
	}
	
	f = fentry_from_path(p, src_path);
	if (!f)
	{
		printf("source: %s, does not exist\n", src_path);
		return 0;
	}
	
	
	
	
	return 1;
}

void cmd_cat(part_info *p, char* path)
{
	f_entry *f = fentry_from_path(p, path);

	char buffer[512];
	unsigned int total_bytes_read = 0;
	unsigned int bytes_read = read_file(p, f, buffer, 512, 0);
	
	while (bytes_read)
	{
		total_bytes_read += bytes_read;
	
		fwrite(buffer, bytes_read, 1, stdout);
		
		bytes_read = read_file(p, f, buffer, 512, total_bytes_read);
	}	
}

void cmd_info(part_info *p)
{
	printf("OEM String: %s\n", p->oem);

//	printf("Bytes/Sector: %#x\n", p.bytes_per_sector);
	printf("Sectors/Cluster: %#x\n", p->sectors_per_cluster);
	printf("Reserved Sectors: %#x\n", p->reserved_sectors);
	printf("Num FAT's: %#x\n", p->num_fats);
//	printf("Max root entries: %#x\n", p->max_root_entries);
	printf("Total Sectors: %#x\n", p->total_sectors);
//	printf("Media: %#x\n", p->media);
	printf("Sectors/Fat: %#x\n", p->sectors_per_fat);
	printf("Root Cluster #: %#x\n", p->root_first_cluster);
//	printf("FS Info Sector #: %#x\n", p.fs_info_sector);
	printf("Volume Label: %s\n", p->label);
	printf("Free Clusters: %#x\n", p->free_clusters);
	printf("Partition Size: ");
	print_human_size(p->total_sectors * p->bytes_per_sector);
	printf(" -");
	print_human_size(p->reserved_sectors+(p->num_fats*p->sectors_per_fat)*p->bytes_per_sector);
	printf(" for filesystem\n");
	printf("%.2f%% full\n", 100.0 - 100.0 * (float)p->free_clusters * (float)p->sectors_per_cluster / (float)(p->total_sectors-p->reserved_sectors-(p->num_fats*p->sectors_per_fat)));
}

#if 0
unsigned int count_unused_cluster_not_zerod(FILE* f, part_info p)
{
	char* read = (char*)malloc(p.bytes_per_cluster);
	char* zeros = (char*)malloc(p.bytes_per_cluster);
	bzero(zeros, p.bytes_per_cluster);

	unsigned int unused = 0;
	unsigned int unused_zerod = 0;

	unsigned int last_cluster = p.sectors_per_fat * p.bytes_per_sector / 4.0;
	printf("total clusters: %#x\n", last_cluster-2);
	unsigned int cur_cluster; // = 2; /* firt 2 fat slots unused */
	for (cur_cluster = 2; cur_cluster <= last_cluster; cur_cluster++)
	{
		unsigned int nxt_cluster = read_fat(f, p, cur_cluster);
		if (!nxt_cluster) /* marked as unused */
		{
			read_cluster(f, p, read, cur_cluster);
			if (memcmp(read, zeros, p.bytes_per_cluster)==0)
			{
				unused_zerod++; /* contains non-zero'd data */
				//printf("*");
//				printf("%d\n", 100*cur_cluster/(last_cluster-1));
			}
			unused++;
//			printf(" ");
		} else {
//			printf(".");
		}
	}

	free(zeros);
	free(read);
	
	return unused-unused_zerod;
}

void zero_unused_clusters(FILE* f, part_info p)
{
	char* zeros = (char*)malloc(p.bytes_per_cluster);
	bzero(zeros, p.bytes_per_cluster);
	
	unsigned int total_clusters = p.sectors_per_fat * p.bytes_per_sector / 4.0;
	unsigned int cur_cluster; // = 2; /* firt 2 fat slots unused */
	for (cur_cluster = 2; cur_cluster < total_clusters; cur_cluster++)
	{
		unsigned int nxt_cluster = read_fat(f, p, cur_cluster);
		if (!nxt_cluster)
		{
			write_cluster(f, p, zeros, cur_cluster);		
		}
	}

	free(zeros);
}

#endif

part_info vfat_init(FILE* f, unsigned int partition_start_sector) {
	part_info p;
	p.f = f;
	p.bytes_per_sector = 0x200; /* fixed? */
	p.first_sector = partition_start_sector;

	char buffer[p.bytes_per_sector];	
	read_sector(&p, buffer, 0);
	
	memcpy(&p.oem, buffer+3, 8);
	p.oem[8] = 0;
	memcpy(&p.label, buffer+0x47, 11);
	p.label[11] = 0;

	p.bytes_per_sector = *(unsigned short*)(buffer+0x0b);
	p.sectors_per_cluster = (unsigned char)buffer[0x0d];
	p.reserved_sectors = *(unsigned short*)(buffer+0x0e);
	p.num_fats = (unsigned char)buffer[0x10];
	p.max_root_entries = *(unsigned short*)(buffer+0x11);
	p.total_sectors = *(unsigned short*)(buffer+0x13);
	if (!p.total_sectors) p.total_sectors = *(unsigned int*)(buffer+0x20);
	p.media = (unsigned char)buffer[0x15];
	p.sectors_per_fat = *(unsigned int*)(buffer+0x24);
	p.root_first_cluster = *(unsigned int*)(buffer+0x2c);
	p.fs_info_sector = *(unsigned short*)(buffer+0x30);
	p.bytes_per_cluster = p.sectors_per_cluster * p.bytes_per_sector;

	p.fat_first_sector = p.reserved_sectors;
	p.clusters_first_sector = p.fat_first_sector + (p.num_fats * p.sectors_per_fat);
	
	read_sector(&p, buffer, p.fs_info_sector);
	p.free_clusters = *(unsigned int*)(buffer+0x1e8);

	return p;
}

int main(int argc, char *argv[]) {	
	FILE* f = fopen(argv[1], "r+");

	unsigned int lba = strtol(argv[2],0,0);
//	printf("using lba: %#x\n", lba);
	part_info p;
	p = vfat_init(f, lba);
	
	char* cmd = argv[3];
	if (strcmp(cmd, "cat") == 0) {
		cmd_cat(&p, argv[4]);
	} else if (strcmp(cmd, "ls") == 0) {
		cmd_ls(&p, argv[4]);
	} else if (strcmp(cmd, "mv") == 0) {
		cmd_mv(&p, argv[4], argv[5]);
	} else if (strcmp(cmd, "mkdir") == 0) {
		cmd_mkdir(&p, argv[4]);
	} else if (strcmp(cmd, "info") == 0) {
		cmd_info(&p);
	} else {
		printf("unknown command!\n");
	}

	fclose(f);
	
	return 0;
}
