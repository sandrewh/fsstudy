#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "ext2.h"
#include "inode.h"

#define READ_WRITE_BUFFER_SIZE 512
#define LS_FILES_BUFFER_SIZE 16

void print_human_size(float size)
{
/*
	if (!size) 
	{
		printf("%8d", 0);
		return;
	}
*/	
	char a = 'T';
	if (size < 1024.0) a = 'B'; 
	else if ((size/=1024.0) < 1024.0) a = 'K';
	else if ((size/=1024.0) < 1024.0) a = 'M';
	else if ((size/=1024.0) < 1024.0) a = 'G';
	else size/=1024.0;

	printf("%6.1f%c ", size, a);
}

void cmd_read(ext2_info *ext2, int argc, char *argv[])
{
	if (argc < 1)
	{
		printf("ls: error, no path given\n");
		return;
	}	
	
	char *path = argv[0];
	directory_entry *d = path_to_entry(ext2, path);
	
	if (!d)
	{
		printf("file not found: %s\n", path);
		return;
	}
	
	if (d->file_type == EXT2_FT_DIR)
	{
		fprintf(stderr, "warning: %s is a directory.\n", path);
//		free(d);
//		return;
	}

	char buffer[READ_WRITE_BUFFER_SIZE];
	unsigned int total_bytes_read = 0;
	unsigned int bytes_read = read_file(ext2, d, buffer, READ_WRITE_BUFFER_SIZE, 0);

	while (bytes_read)
	{
		total_bytes_read += bytes_read;
	
		fwrite(buffer, bytes_read, 1, stdout);

		bytes_read = read_file(ext2, d, buffer, READ_WRITE_BUFFER_SIZE, total_bytes_read);
	}
	
	free(d);
}

void cmd_ls(ext2_info *ext2, int argc, char *argv[])
{
	if (argc < 1)
	{
		printf("ls: error, no directory given\n");
		return;
	}
	
	char *path = argv[0];	
	directory_entry *d = path_to_entry(ext2, path);

	if (!d)
	{
		printf("Directory %s not found.\n", path);
		return;
	}

	if (d->file_type != EXT2_FT_DIR)
	{
		printf("%s is not a directory.\n", path);
		free(d);
		return;
	}

	printf("Directory listing of %s (inode:%x)\n", path, d->inode);

	inode in;
	inode_read(ext2, &in, d->inode);	

	free(d->name);
	free(d);

	directory_entry entries[LS_FILES_BUFFER_SIZE];
	unsigned int total_entries_read = 0;
	unsigned int entries_read = read_dir(ext2, &in, entries, LS_FILES_BUFFER_SIZE, 0);

	while (entries_read)
	{
		total_entries_read += entries_read;
		
		unsigned int i;
		for (i=0;i<entries_read;i++)
		{

			inode f_in;
			inode_read(ext2, &f_in, entries[i].inode);	
			
			// TODO: use 64-bit size
			/* TODO: set process uid */
			/* TODO: set process gid */
			/* TODO: sticky bit */			
			
			if (f_in.i_mode & EXT2_S_IFDIR) printf("d");
			else printf("-");
			if (f_in.i_mode & EXT2_S_IRUSR) printf("r");
			else printf("-");
			if (f_in.i_mode & EXT2_S_IWUSR) printf("w");
			else printf("-");
			if (f_in.i_mode & EXT2_S_IXUSR) printf("x");
			else printf("-");
			if (f_in.i_mode & EXT2_S_IRGRP) printf("r");
			else printf("-");
			if (f_in.i_mode & EXT2_S_IWGRP) printf("w");
			else printf("-");
			if (f_in.i_mode & EXT2_S_IXGRP) printf("x");
			else printf("-");
			if (f_in.i_mode & EXT2_S_IROTH) printf("r");
			else printf("-");
			if (f_in.i_mode & EXT2_S_IWOTH) printf("w");
			else printf("-");
			if (f_in.i_mode & EXT2_S_IXOTH) printf("x");
			else printf("-");

			printf(" %2u ", f_in.i_links_count);
			if (!f_in.i_uid) printf("root ");
			else printf("%4u ", f_in.i_uid);
			if (!f_in.i_gid) printf("root ");
			else printf("%4u ", f_in.i_gid);
			print_human_size(f_in.i_size);
			char *time = ctime((time_t*)&(f_in.i_mtime));
			time[24] = 0;
			printf("%s ", time+4);			

			printf("%s\n", entries[i].name);
			free(entries[i].name);
		}

		entries_read = read_dir(ext2, &in, entries, LS_FILES_BUFFER_SIZE, total_entries_read);
	}

	printf("\n%d entries.\n", total_entries_read);	
}

int main(int argc, char *argv[]) {
	char *path = 0;
	unsigned int lba = 0x0;
	char **sub_argv = 0;
	int num_sub_args = 0;
	
	enum param {
		PARAM_NONE = 0,
		PARAM_LBA_OFFSET = 1,
		PARAM_IMAGE_PATH = 2,
		PARAM_CMD_ARG = 3
	} next_param;
	
	next_param = PARAM_NONE;
	
	int i;
	for (i=1;i<argc;i++)
	{
		switch (next_param)
		{
			case PARAM_NONE:
				if (strcmp(argv[i], "-o") == 0)
				{
					next_param = PARAM_LBA_OFFSET;
				} else if (strcmp(argv[i], "-f") == 0) {
					next_param = PARAM_IMAGE_PATH;					
				} else {
					next_param = PARAM_CMD_ARG;
					sub_argv = malloc(sizeof(char*)*(argc-i));					
					i--;
				}
				break;
			case PARAM_LBA_OFFSET:
				lba = strtol(argv[i], 0, 0);
				next_param = PARAM_NONE;
				break;
			case PARAM_IMAGE_PATH:
				path = argv[i];
				next_param = PARAM_NONE;
				break;
			case PARAM_CMD_ARG:
				sub_argv[num_sub_args++] = argv[i];
				break;
		}
	}
	
	FILE* f = fopen(path, "r");
	if (!f)
	{
		printf("fopen: failed opening %s\n", path);
		return 1;
	}

	ext2_info *ext2;
	ext2 = ext2_mount(f, lba);

	if (ext2)
	{
		if (num_sub_args)
		{
			if (strcmp(sub_argv[0], "ls") == 0) {
				cmd_ls(ext2, num_sub_args-1, sub_argv+1);
			} else if (strcmp(sub_argv[0], "read") == 0) {
				cmd_read(ext2, num_sub_args-1, sub_argv+1);
			} else {
				printf("unknown command: %s!\n", sub_argv[0]);
			}				
		} else {
			printf("no command given\n");
		}

		ext2_umount(ext2);		
	}
	
	if (sub_argv) free(sub_argv);

	fflush(f);
	fclose(f);
	
	return 0;
}
