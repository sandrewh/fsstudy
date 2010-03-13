#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "vfat.h"

/*
 *
 * READ ONLY FUNCTIONS
 *
 */

f_entry*
fentry_from_path(part_info *p, char *path)
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
			break;
		}
		
		if (!parent->attr_d) 
		{
			parent = 0;
			break;
		}
		
		child = search_dir(p, parent->first_cluster, search_name);
		
		if (!child)
		{
			parent = 0;
			break;
		}
		
		free(parent);
		parent = child;
	}
	
	free(path);
	return parent;
}

f_entry*
enumerate_dir(part_info* p, uint32_t dir_cluster, int callback(f_entry f, void *arg), void *arg)
{
	unsigned int entry_num = 0;
	char entry[FILE_ENTRY_SIZE];
	f_entry file;
	file.lfn[0] = 0;
	file.dir_num_slots = 0;
	uint8_t last_lfn_checksum = 0;
	
	while (read_cluster_chain(p, dir_cluster, entry, FILE_ENTRY_SIZE, entry_num++ * FILE_ENTRY_SIZE))
	{
		if (!entry[0]) break; /* end of listing */
		if ((uint8_t)entry[0] == 0xe5)
		{
			file.lfn[0] = 0;
			file.dir_num_slots = 0;			
			continue; // marked unused
		}
	
		memcpy(file.name, entry, 11);
		file.name[11] = file.name[12] = 0;

		// translate 0x05 to value 0xe5
		file.name[0] = (file.name[0] == 0x05 ? 0xe5 : file.name[0]);

		f_entry_set_attr(&file, entry[11]);
		f_entry_set_datetimes(&file, entry);
		file.dir_num_slots++;
		
		// LFN entry?
		if (file.attr_r && file.attr_h && file.attr_s && file.attr_v) {
			if (entry[0] & 0x80) continue; // a deleted lfn entry
			last_lfn_checksum = entry[0x0d];
			f_entry_update_lfn(&file, entry);
			continue;
		}

		// Volume name?
		if (file.attr_v)
		{
			file.lfn[0] = 0;
			file.dir_num_slots = 0;						
			continue;
		}
		
		uint16_t cluster_high, cluster_low;
		cluster_high = *(unsigned short*)(entry+0x14);
		cluster_low = *(unsigned short*)(entry+0x1a);
		file.size_bytes = *(unsigned int*)(entry+0x1c);
		file.first_cluster = (((unsigned int)cluster_high) << 16) | ((unsigned int)cluster_low);
		
		/* if there was an LFN */
		if (file.lfn[0])
		{
			/* if the LFN's checksum doesn't match the filename */
			if (last_lfn_checksum != lfn_checksum((unsigned char*)file.name))
			{
				/* ignore the LFN */
				file.lfn[0] = 0;
			}			
		}
		
		f_entry_set_name_from_8_3(&file);
		
		file.dir_slot_offset = entry_num-1; /* points to 8.3 name (last slot), not LFN entries */
		file.dir_first_cluster = dir_cluster;		

		if (!callback(file, arg))
		{
			f_entry *f = (f_entry*)malloc(sizeof(f_entry));
			return memcpy(f, &file, sizeof(f_entry));
		}

		file.lfn[0] = 0;
		file.dir_num_slots = 0;
	} /* while */

	return 0;
}

int
search_dir_callback(f_entry file, void *arg)
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

f_entry*
search_dir(part_info *p, uint32_t dir_cluster, char* search_name)
{
	return enumerate_dir(p, dir_cluster, search_dir_callback, (void*)search_name);
}

int
read_file(part_info *p, f_entry *f, char* buffer, size_t size, off_t offset)
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
		
	size = read_cluster_chain(p, f->first_cluster, buffer, size, offset);

	return size;
}

typedef struct {
	uint32_t entry_num;
	off_t offset;
	size_t size;
	uint32_t num_read;
	f_entry *entries;
} search_dir_data;

int
read_dir_callback(f_entry file, void *arg)
{
	search_dir_data *data = arg;
	
	if (data->entry_num++ < data->offset) return 1; /* keep going */
	if (data->num_read >= data->size) return 0; /* stop */

	data->entries[data->num_read++] = file;
	
	return 1; /* keep going */
}

unsigned int
read_dir(part_info *p, char* path, f_entry *entries, size_t size, off_t offset)
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
	data.entries = entries;//(f_entry*)malloc(size*sizeof(f_entry));

	enumerate_dir(p, f->first_cluster, read_dir_callback, (void*)&data);

//	memcpy(entries, data.entries, data.num_read * sizeof(f_entry));
//	free(data.entries);

	return data.num_read;
}

#if 0

void
count_unused_cluster_not_zerod(part_info *p)
{
	char* read = (char*)malloc(p->bytes_per_cluster);
	char* zeros = (char*)malloc(p->bytes_per_cluster);
	bzero(zeros, p->bytes_per_cluster);

	unsigned int unused = 0;
	unsigned int unused_zerod = 0;

	unsigned int last_cluster = p->num_data_clusters+2;
	printf("total clusters: %#x\n", last_cluster-2);
	unsigned int cur_cluster; // = 2; /* first 2 fat slots unused */
	for (cur_cluster = 2; cur_cluster <= last_cluster; cur_cluster++)
	{
		unsigned int nxt_cluster = read_fat(p, cur_cluster);
		if (!nxt_cluster) /* marked as unused */
		{
			read_cluster(p, read, cur_cluster);
			if (memcmp(read, zeros, p->bytes_per_cluster)==0)
			{
				unused_zerod++; /* contains non-zero'd data */
//				printf("*");
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
	
	printf("%#x\n", unused-unused_zerod);
}

// TODO: zero_unused_clusters: can also zero all data at end of cluster chains, but < file size or after directory end

void zero_unused_clusters(part_info *p)
{
	char* zeros = (char*)malloc(p->bytes_per_cluster);
	memset(zeros, 0, p->bytes_per_cluster);
	
	unsigned int last_cluster = p->num_data_clusters+2;
	unsigned int cur_cluster; // = 2; /* first 2 fat slots unused */
	for (cur_cluster = 0; cur_cluster <= last_cluster; cur_cluster++)
	{
		unsigned int nxt_cluster = read_fat(p, cur_cluster);
		if (!nxt_cluster) write_cluster(p, zeros, cur_cluster);
	}

	free(zeros);
}
#endif

/*
 *
 * WRITE ONLY FUNCTIONS
 *
 */

void
file_size_to_disk(part_info *p, f_entry *f)
{
	write_cluster_chain(p, f->dir_first_cluster, (char*)&f->size_bytes, 4, (f->dir_slot_offset*FILE_ENTRY_SIZE)+0x1c);
}

void
first_cluster_to_disk(part_info *p, f_entry *f)
{
//	printf("first_cluster_to_disk: FILE_ENTRY_SIZE: %d, f->dir_slot_offset: %d\n", FILE_ENTRY_SIZE, f->dir_slot_offset);
	
	char entry[FILE_ENTRY_SIZE];
	read_cluster_chain(p, f->dir_first_cluster, entry, FILE_ENTRY_SIZE, f->dir_slot_offset*FILE_ENTRY_SIZE);

	/* msb */
	entry[0x14] = (f->first_cluster >> 16) & 0xff; /* lsb */
	entry[0x15] = f->first_cluster >> 24; /* msb */

	/* lsb */
	entry[0x1a] = f->first_cluster & 0xff; /* lsb */
	entry[0x1b] = (f->first_cluster >> 8) & 0xff; /* msb */

	write_cluster_chain(p, f->dir_first_cluster, entry, FILE_ENTRY_SIZE, f->dir_slot_offset*FILE_ENTRY_SIZE);
}

int
write_file(part_info *p, f_entry *f, char* buffer, size_t size, off_t offset)
{
	/* if not clusters allocated for file (empty file) */
	if (!f->first_cluster)
	{
		/* allocate a cluster */
		uint32_t new_cluster = allocate_new_cluster(p, 0);
		if (!new_cluster) return 0;

//		printf("write_file: first_cluster empty, allocated new one: %#x\n", new_cluster);
		
		f->first_cluster = new_cluster;
		first_cluster_to_disk(p, f);
	}

	return write_cluster_chain(p, f->first_cluster, buffer, size, offset);
	return 0;
}

// should set 0x80 bit in index 0 of all LFN entry
// should set entry[0]=0xe5 of 8.3 entry
int
dir_rem_entry(part_info *p, f_entry *f)
{
//	printf("dir_rem_entry: f->name: %s, f->dir_first_cluster: %#x, f->dir_slot_offset: %d, f->dir_num_slots: %d\n", f->name, f->dir_first_cluster, f->dir_slot_offset, f->dir_num_slots);
	
	/* handle 8.3 entry */
	char deleted = 0xe5; /* tested! */
	write_cluster_chain(p, f->dir_first_cluster, &deleted, 1, (f->dir_slot_offset*FILE_ENTRY_SIZE)+0x00);

	/* handle LFN entries */
	uint32_t slot; /* tested when num_slots = 2 */
	for (slot = f->dir_slot_offset - (f->dir_num_slots - 1); slot < (f->dir_slot_offset); slot++)
	{
		read_cluster_chain(p, f->dir_first_cluster, &deleted, 1, (slot*FILE_ENTRY_SIZE)+0x00);
		deleted |= 0x80;
		write_cluster_chain(p, f->dir_first_cluster, &deleted, 1, (slot*FILE_ENTRY_SIZE)+0x00);
	}

	return 1;
}

int
dir_add_entry(part_info *p, f_entry *d, f_entry* f)
{
	unsigned int entry_num = 0;
	char entry[FILE_ENTRY_SIZE];
	f_entry file;
	file.lfn[0] = 0;
	uint32_t consecutive_unused = 0;
	uint32_t slots_required = ceil((float)(strlen(f->lfn))/13.0)+1; /* 13 chars / slot */
	
//	printf("adding lfn:%s; name:%s, slots:%d\n", f->lfn, f->name, slots_required);
	
	int grew_directory = 0;
	
	while (read_cluster_chain(p, d->first_cluster, entry, FILE_ENTRY_SIZE, entry_num*FILE_ENTRY_SIZE))
	{		
		if (!entry[0]) /* end of listing */
		{
//			printf("eod found, consecutive_unused: %d\n", consecutive_unused);
			consecutive_unused++;			
			grew_directory = 1;
			break;
		}
		
		if (((unsigned char)entry[0] != 0xe5) // 8.3 entry already in use
			&& !((unsigned char)entry[0x0b] == 0x0f /* lfn */
				&& (unsigned char)entry[0x00] & 0x40))  /* unused lfn */
		{
			consecutive_unused = 0;

//			printf("%d: in use, consecutive_unused: %d\n", entry_num, consecutive_unused);
			
			entry_num++;
			continue;
		}
		
		consecutive_unused++;

//		printf("%d: FREE, consecutive_unused: %d\n", entry_num, consecutive_unused);

		if (consecutive_unused < slots_required)
		{			
			entry_num++;
			continue;// need more slots			
		}
		
		/* have enough slots */
		break;
	} /* while */

	//update parent dir relationships
	f->dir_first_cluster = d->first_cluster;
	f->dir_slot_offset = entry_num - (consecutive_unused + 1) + (slots_required + 1);
	f->dir_num_slots = slots_required;
	
	/* put new entry at (entry_num-consecutive_unused+1)*FILE_ENTRY_SIZE */
//	printf("dir_add_entry: insert new entry and eod @ %d.\n", f->dir_slot_offset);
	
	// if the 8.3 entry is blank, create it from the lfn
	if (!f->name[0])
	{
		fentry_set_unique_name_from_lfn(f);
	}
	
	fentry_convert_name_to_8_3(f);
	
	uint8_t name_checksum = lfn_checksum((unsigned char*)f->name);

	// the LFN entries
	int slot;
	for (slot=0;slot<slots_required-1;slot++)
	{
		memset(entry, 0, FILE_ENTRY_SIZE);
		uint32_t seq_num = slots_required-slot-1;
		if (!slot) seq_num |= 0x40;
		entry[0] = seq_num;
		entry[0x0b] = 0x0f;
		entry[0x0d] = name_checksum;

		char lfn_part[26];
		strncpy(lfn_part, f->lfn+13*slot, 13);
		to_utf16(lfn_part, strlen(lfn_part)+1, 26);
		
		memcpy(entry+0x01, lfn_part, 10);
		memcpy(entry+0x0e, lfn_part+10, 12);
		memcpy(entry+0x1c, lfn_part+22, 4);
/*
		int i;
		for (i=0;i<FILE_ENTRY_SIZE;i++)
		{
			printf("%#02x ", (unsigned char)entry[i]);
		}
		printf("\n");
*/		
		/* write new lfn entry to disk */
//		printf("writing lfn entry %d @ slot %d\n", slot, (f->dir_slot_offset-(slots_required-slot-1)));
		write_cluster_chain(p, d->first_cluster, entry, FILE_ENTRY_SIZE, (f->dir_slot_offset-(slots_required-slot-1))*FILE_ENTRY_SIZE);		
	}

	// TODO: dir_add_entry: test this function
	// especially: the offsets for the 2 write_cluster_chains
	// also: the code below ...
		
	/* the 8.3 entry */
	strcpy(entry, f->name);

	// translate 0xe5 to value 0x05
	entry[0] = ((unsigned char)entry[0] == 0xe5 ? 0x05 : entry[0]);
	// encode f_entry			
	entry[0x0b] = f->attr_r | f->attr_h | f->attr_s | f->attr_v | f->attr_d | f->attr_a;
	// TODO: dir_add_entry: encode times
	memset(entry+0x0c, 0, 0x14);
	
	/* msb */
	entry[0x14] = (f->first_cluster >> 16) & 0xff; /* lsb */
	entry[0x15] = f->first_cluster >> 24; /* msb */

	/* lsb */
	entry[0x1a] = f->first_cluster & 0xff; /* lsb */
	entry[0x1b] = (f->first_cluster >> 8) & 0xff; /* msb */	

	/* the file size */
	memcpy(&entry[0x1c], &f->size_bytes, 4);
	
//	printf("writing 8.3 entry @ slot %d\n", f->dir_slot_offset);
	/* write the 8.3 entry to disk */
	write_cluster_chain(p, d->first_cluster, entry, FILE_ENTRY_SIZE, f->dir_slot_offset*FILE_ENTRY_SIZE);
	
	if (grew_directory)
	{
		/* if overwrote the end of directory, append a new one */
		char zero = 0;
		write_cluster_chain(p, d->first_cluster, &zero, 1, (f->dir_slot_offset+1)*FILE_ENTRY_SIZE);		
	}
	
	return 1;
}

/* for files or dir's */
// mark entire cluster chain as empty
// remove entry from containing directory
int
file_rem(part_info *p, char *path)
{
	f_entry *f = fentry_from_path(p, path);
	if (!f)
	{
		printf("%s does not exist.\n", path);
		return 0;
	}
	dir_rem_entry(p, f);
	truncate_cluster_chain(p, f->first_cluster, 0);
	return 1;
}

int
entry_create(part_info *p, char *path, unsigned char attr)
{
	// does the entry already exist? 
	f_entry *d = fentry_from_path(p, path);
	if (d)
	{
		printf("path %s already exists.\n", path);
		free(d);
		return 0;
	}

	path = strdup(path); /* don't destroy original path */
	/* get the f_entry of the path leading up to the last '/' */
	char* last_slash = strrchr(path, '/');
	/* everything after last slash is _name_ */
	char* name = strdup(last_slash+1);
	last_slash[0] = 0; /* termiante path at last slash */
	d = fentry_from_path(p, path);
	
	f_entry f;
	f.first_cluster = 0; /* empty file (or dir) */
	f.size_bytes = 0;
	
	strcpy(f.lfn, name);
	f.name[0] = 0;

	/* TODO: entry_create: use realtime datetimes */
	char blank_datetime[0x20];
	bzero(blank_datetime, 0x20);

	f_entry_set_attr(&f, attr);			
	f_entry_set_datetimes(&f, blank_datetime);
	
	if (f.attr_d)
	{
		f.first_cluster = allocate_new_cluster(p, 0);
		if (!f.first_cluster)
		{
			free(name);
			free(d);
			free(path);
			return 0;			
		}
		char zero = 0;
		write_file(p, &f, &zero, 1, 0);
	} else {
		f.first_cluster = 0;
	}

	int ret = dir_add_entry(p, d, &f);
	free(name);
	free(d);
	free(path);
	return ret;
}

/*
 *
 * Mount Functions
 *
 */

part_info*
vfat_mount(FILE* f, unsigned int partition_start_sector)
{
	part_info* p = malloc(sizeof(part_info));
	p->f = f;
	p->bytes_per_sector = 0x200; /* for now ... */
	p->first_sector = partition_start_sector;

	char * buffer = malloc(p->bytes_per_sector);
//	char buffer[p->bytes_per_sector];
	read_sector(p, buffer, 0);
	
	memcpy(&(p->oem), buffer+3, 8);
	p->oem[8] = 0;
	memcpy(&(p->label), buffer+0x47, 11);
	p->label[11] = 0;

	p->bytes_per_sector = *(unsigned short*)(buffer+0x0b);
	p->sectors_per_cluster = (unsigned char)buffer[0x0d];
	p->reserved_sectors = *(unsigned short*)(buffer+0x0e);
	p->num_fats = (unsigned char)buffer[0x10];
	p->max_root_entries = *(unsigned short*)(buffer+0x11);
	p->total_sectors = *(unsigned short*)(buffer+0x13);
	if (!p->total_sectors) p->total_sectors = *(unsigned int*)(buffer+0x20);
	p->media = (unsigned char)buffer[0x15];
	p->sectors_per_fat = *(unsigned int*)(buffer+0x24);
	p->root_first_cluster = *(unsigned int*)(buffer+0x2c);
	p->fs_info_sector = *(unsigned short*)(buffer+0x30);
	p->bytes_per_cluster = p->sectors_per_cluster * p->bytes_per_sector;

	p->fat_first_sector = p->reserved_sectors;
	p->clusters_first_sector = p->fat_first_sector + (p->num_fats * p->sectors_per_fat);
	
	free(buffer);
	if (p->root_first_cluster != 2)
	{
		free(p);
		printf("vfat_mount: rout_first_cluster != 2\ndid you set the partition offset with -o?\n");
		return NULL;
	}
	
	/* use the new bytes_per_sector number */	
	buffer = malloc(p->bytes_per_sector);
	
	read_sector(p, buffer, p->fs_info_sector);
	p->free_clusters_on_mount = *(unsigned int*)(buffer+0x1e8);
	p->free_clusters = p->free_clusters_on_mount;
	
	p->num_data_clusters = p->total_sectors - p->reserved_sectors - (p->num_fats*p->sectors_per_fat);
	p->last_allocated_cluster_on_mount = *(unsigned int*)(buffer+0x1ec);
	p->last_allocated_cluster = p->last_allocated_cluster_on_mount;
	
	free(buffer);
	
#if USE_CACHE
	cache_init(p);
#endif
	
	return p;
}

void
vfat_umount(part_info *p)
{
	if (p->free_clusters != p->free_clusters_on_mount)
	{
		// write p->free_clusters back to disk
		char buffer[p->bytes_per_sector];
		read_sector(p, buffer, p->fs_info_sector);
		*(uint32_t*)(buffer+0x1e8) = p->free_clusters;
		write_sector(p, buffer, p->fs_info_sector);
	}
	
	if (p->last_allocated_cluster != p->last_allocated_cluster_on_mount)
	{
		// write p->free_clusters back to disk
		char buffer[p->bytes_per_sector];
		read_sector(p, buffer, p->fs_info_sector);
		*(uint32_t*)(buffer+0x1ec) = p->last_allocated_cluster;
		write_sector(p, buffer, p->fs_info_sector);
	}
	
#if USE_CACHE
	cache_flush(&p);
#endif
}
