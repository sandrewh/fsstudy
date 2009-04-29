#include <stdlib.h>
#include <string.h>

#include "ext2.h"
#include "inode.h"
#include "block.h"

/* TODO: inode bitmap */
/* TODO: block bitmap */
/* TODO: sym links */
/* TODO: directory indexing  */

//http://www.nongnu.org/ext2-doc/ext2.html

const unsigned int BYTES_PER_SECTOR = 512;
const char PATH_SEPARATOR[] = "/";
const unsigned int PATH_SEPARATOR_LEN = 1;
const unsigned int EXT2_SUPER_BLOCK_MAGIC = 0xef53;
const unsigned int EXT2_VALID_FS = 1;

// TODO: use same block enumerating mechanism (w/ callbacks?) for enumerate_dir and read_file?

void enumerate_dir(ext2_info *ext2, inode *in, int callback(directory_entry entry, void *arg), void *arg)
{
	// TODO: upper 32-bits of size in in->i_dir_acl
	// TODO: handle indirect blocks

	if (in->i_block[12])
	{
		printf("enumerate_dir: indirect blocks not supported!\n");
		exit(1);
	}

//	printf("enumerate_dir: blocks: %u, size: %u\n", in->i_blocks, in->i_size);
	int i;
	for (i=0;i<15;i++)
	{
		if (!in->i_block[i]) break;
	}

	unsigned int used_blocks = in->i_size / ext2->bytes_per_block;

	unsigned int block_num;
	for (block_num=0; block_num < used_blocks; block_num++)
	{
		char block[ext2->bytes_per_block];
		block_read(ext2, block, in->i_block[block_num]);

		directory_entry entry;

		/* enumerate the entries in the block */
		char *cur_entry = block;
		for (;;)
		{
			entry = *(directory_entry*)cur_entry;
			entry.name = malloc(entry.name_len + 1);
			strncpy(entry.name, cur_entry+8, entry.name_len); /* entry name is not 0-terminated */
			entry.name[entry.name_len] = 0;
			
			int ret = callback(entry, arg);
			free(entry.name); entry.name = 0;
			if (!ret) return;

			cur_entry += entry.rec_len;

			if (cur_entry >= block + ext2->bytes_per_block) break;
		}		
	}

	return;
}

typedef struct {
	char *search_name;		/* name to search for */
	directory_entry *entry;	/* pointer to result (0 if not found) */
} search_dir_data;

/* find and return 1 directory entry for search_dir */
int search_dir_callback(directory_entry entry, void *arg)
{
	search_dir_data *data = (search_dir_data*)arg;

	/* search name found? */
	if (strcmp(entry.name, data->search_name) == 0)
	{
		entry.name = strdup(entry.name); /* keep our own copy of filename - caller should free this one */
		data->entry = malloc(sizeof(directory_entry)); /* create space to store the result */
		*(data->entry) = entry; /* copy result */
		return 0; /* stop enumerating */
	}

	return 1; /* continue enumerating */
}

directory_entry* search_dir(ext2_info *ext2, inode *in, char* search_name)
{
	search_dir_data data;

	data.search_name = search_name;	/* see def: search_dir_data for info */
	data.entry = 0;	/* nothing found, yet */

	enumerate_dir(ext2, in, search_dir_callback, (void*)&data);

#if 0
	enumerate_dir(ext2, in, (void*)&data) : int (directory_entry entry, void *arg) {
		search_dir_data *data = (search_dir_data*)arg;

		/* search name found? */
		if (strcmp(entry.name, data->search_name) == 0)
		{
			entry.name = strdup(entry.name); /* keep our own copy of filename - caller should free this one */
			data->entry = malloc(sizeof(directory_entry)); /* create space to store the result */
			*(data->entry) = entry; /* copy result */
			return 0; /* stop enumerating */
		}

		return 1; /* continue enumerating */
	}
#endif

	return data.entry;
}

typedef struct {
	unsigned entry_num;			/* # entries read total */
	unsigned offset;			/* # entries to skip */
	unsigned size;				/* # entires to copy */
	unsigned int num_read;		/* # entries copied total */
	directory_entry *entries;	/* buffer to copy entires to */
} read_dir_data;

/* copy certain span of directory entries into buffer for read_dir */
int read_dir_callback(directory_entry entry, void *arg)
{
	read_dir_data *data = arg;

	if (data->entry_num++ < data->offset) return 1; /* keep going */
	if (data->num_read >= data->size) return 0; /* stop */

	entry.name = strdup(entry.name); /* keep our own copy of filename - caller should free this one */
	data->entries[data->num_read++] = entry;

	return 1; /* keep going */
}

/* copy certain span of directory entries into /entries/ for caller */
unsigned int read_dir(ext2_info *ext2, inode *in, directory_entry *entries, size_t size, off_t offset)
{	
	read_dir_data data;

	data.entry_num = 0;		/* see def: read_dir_data for info */
	data.offset = offset;
	data.size = size;
	data.num_read = 0;
	data.entries = entries;

	enumerate_dir(ext2, in, read_dir_callback, (void*)&data);

#if 0
	enumerate_dir(ext2, in, (void*)&data) : int (directory_entry entry, void *arg) {	
		read_dir_data *data = arg;

		if (data->entry_num++ < data->offset) return 1; /* keep going */
		if (data->num_read >= data->size) return 0; /* stop */

		entry.name = strdup(entry.name); /* keep our own copy of filename - caller should free this one */
		data->entries[data->num_read++] = entry;

		return 1; /* keep going */
	}
#endif

	return data.num_read;
}

/* OPT: path_to_entry: make this recursive? */
directory_entry* path_to_entry(ext2_info *ext2, char *path)
{
	path = strdup(path); /* don't mess w/ original path */
	char *search_name = strtok(path, PATH_SEPARATOR);

	directory_entry *parent, *child = 0;
	
	/* return root? */
	if (!search_name)
	{
		directory_entry *root = malloc(sizeof(directory_entry));
		
		root->inode = EXT2_ROOT_INO;
		root->rec_len = 0; /* important? */
		root->name_len = PATH_SEPARATOR_LEN;
		root->file_type = EXT2_FT_DIR;
		root->name = strdup(PATH_SEPARATOR);

		free(path); path = 0;
		return root;
	}
	
	inode in;
	inode_read(ext2, &in, EXT2_ROOT_INO);
	parent = search_dir(ext2, &in, search_name);	

	for (;;)
	{
		/* get the next name to traverse */
		search_name = strtok(0, PATH_SEPARATOR);
		if (!search_name) 
		{
			/* no more path to traverse */
			break;
		}
		
		/* if next name isn't a folder, error */
		if (parent->file_type != EXT2_FT_DIR)
		{
			free(parent->name); parent->name = 0;
			free(parent); parent = 0;
			break;
		}
		
		/* get the inode of the next directory */
		inode_read(ext2, &in, parent->inode);		
		child = search_dir(ext2, &in, search_name);

		/* if it doesn't exist, error */
		if (!child)
		{
			free(parent); parent = 0;
			break;
		}
		
		/* enter the next directory and loop */
		free(parent->name); parent->name = 0;
		free(parent); parent = child;
	}
	
	free(path);
	/* return the last directory entry found (may be NULL) */
	return parent;
}

int read_file(ext2_info *ext2, directory_entry *d, char* buffer, size_t size, off_t offset)
{
	inode in;
	inode_read(ext2, &in, d->inode);

	//TODO: use 64-bit size from inode (make a long long int inode_get_size(...);)

	/* don't start past EOF */
	if (offset >= in.i_size)
		return 0;
	
	/* don't end past EOF */
	if (offset + size > in.i_size)
		size = in.i_size - offset;

	/* allocate space to read a block into */
	char block[ext2->bytes_per_block];

	/* find the block with our offset */		
	unsigned int block_num = offset / ext2->bytes_per_block;

	/* find the byte offset within our block */
	unsigned int block_byte_offset = offset % ext2->bytes_per_block;

	unsigned int bytes_read = 0;
	
	/* read data from DIRECT blocks */
	for (; block_num < 12; block_num++)
	{
		/* read the block */
		block_read(ext2, block, in.i_block[block_num]);

		unsigned int bytes_to_read = ext2->bytes_per_block - block_byte_offset;
		if (size - bytes_read < bytes_to_read) bytes_to_read = size - bytes_read;

		/* copy data from the block into the read buffer */
		memcpy(buffer+bytes_read, block+block_byte_offset, bytes_to_read);
		block_byte_offset = 0;
		bytes_read += bytes_to_read;

		/* stop if read enough data */
		if (bytes_read >= size) return size;
	}
	
	/* read data from SINGLE INDIRECT blocks */	
	unsigned int blockids_per_block = ext2->bytes_per_block / sizeof(unsigned int);
	unsigned int single_indirect_block[blockids_per_block];
	
	block_read(ext2, (char*)&single_indirect_block, in.i_block[12]);
	
	for (; block_num-11 < blockids_per_block; block_num++)
	{
		block_read(ext2, block, single_indirect_block[block_num-12]);
		unsigned int bytes_to_read = ext2->bytes_per_block - block_byte_offset;
		if (size - bytes_read < bytes_to_read) bytes_to_read = size - bytes_read;

//		printf("%d bytes from cluster %d::%d to buffer[%d]\n", bytes_to_read, cur_cluster, cluster_byte_offset, bytes_read);

		memcpy(buffer+bytes_read, block+block_byte_offset, bytes_to_read);
		block_byte_offset = 0;
		bytes_read += bytes_to_read;

		if (bytes_read >= size) return size;		
	}
	
	fprintf(stderr, "enumerate_dir: double/tripple indirect blocks not supported!\n");
	exit(1);
	
	return size;
}

ext2_info* ext2_mount(FILE* f, unsigned int partition_start_sector)
{
	ext2_info *ext2 = malloc(sizeof(ext2_info));
	ext2->volume_offset = partition_start_sector * BYTES_PER_SECTOR;
	ext2->super_block_offset = 1024;
	ext2->f = f;

	if (!super_block_read(ext2))
	{
		printf("ext2_mount: failed: could not read super block\n");
		free(ext2);
		return 0;
	}

	if (ext2->s.s_magic != EXT2_SUPER_BLOCK_MAGIC)
	{
		printf("ext2_mount: failed: super block magic number = %#x (should be %#x)\n", ext2->s.s_magic, EXT2_SUPER_BLOCK_MAGIC);
		free(ext2);
		return 0;
	}
	
	if (ext2->s.s_state != EXT2_VALID_FS)
	{
		printf("ext2_mount: warning: fs was not unmounted cleanly - errors may exist!\n");
	}

	return ext2;
}

void ext2_umount(ext2_info *ext2)
{
	free(ext2); ext2 = 0;
}