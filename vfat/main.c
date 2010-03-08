#include <stdlib.h>
#include <string.h>

#include "main.h"

#define READ_WRITE_BUFFER_SIZE 512
#define LS_FILES_BUFFER_SIZE 16

/*
 *
 * READ ONLY FUNCTIONS
 *
 */

void
print_human_size (float size)
{
	if (!size) 
	{
		printf("%7d", 0);
		return;
	}

	char a = 'T';
	if (size < 1024.0) a = 'B'; 
	else if ((size/=1024.0) < 1024.0) a = 'K';
	else if ((size/=1024.0) < 1024.0) a = 'M';
	else if ((size/=1024.0) < 1024.0) a = 'G';
	else size/=1024.0;

	if (a == 'B')
		printf("%6.0f%c", size, a);
	else
		printf("%6.1f%c", size, a);
}

void
cmd_read (part_info *vfat, int argc, char *argv[])
{
	if (argc < 1)
	{
		printf("read: error, no path given\n");
		return;
	}	
	
	f_entry *f = fentry_from_path(vfat, argv[0]);

	char buffer[READ_WRITE_BUFFER_SIZE];
	unsigned int total_bytes_read = 0;
	unsigned int bytes_read = read_file(vfat, f, buffer, READ_WRITE_BUFFER_SIZE, 0);
	
	while (bytes_read)
	{
		total_bytes_read += bytes_read;
	
		fwrite(buffer, bytes_read, 1, stdout);
		
		bytes_read = read_file(vfat, f, buffer, READ_WRITE_BUFFER_SIZE, total_bytes_read);
	}	
}

void
cmd_ls (part_info *vfat, int argc, char *argv[])
{
	if (argc < 1)
	{
		printf("ls: error, no directory given\n");
		return;
	}	
	
	char * path = argv[0];
	
	f_entry *d = fentry_from_path(vfat, path);
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

	f_entry entries[LS_FILES_BUFFER_SIZE];
	unsigned int total_files_read = 0;
	unsigned int files_read = read_dir(vfat, path, entries, LS_FILES_BUFFER_SIZE, 0);
	
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
			{
				printf(" %02d-%02d-%04d %02d:%02d ",
					entries[i].created.mon,
					entries[i].created.d,
					entries[i].created.y,
					entries[i].created.h,
					entries[i].created.min);
			} else {
				printf(" -                ");
			}
			
			printf("%6x ", entries[i].first_cluster);

			printf("%3x ", entries[i].dir_slot_offset);

			printf("%3x ", entries[i].dir_num_slots);
			
			printf("%-12s  ", entries[i].name);
			
			if (entries[i].lfn[0])
			{
				printf("%s\n", entries[i].lfn);					
			} else {
				printf("%s\n", entries[i].name);				
			}
		}

		files_read = read_dir(vfat, path, entries, LS_FILES_BUFFER_SIZE, total_files_read);
	}
	
	printf("%d entries.\n", total_files_read);
}

/*
 *
 * WRITE FUNCTIONS
 *
 */

void
cmd_write (part_info *vfat, int argc, char *argv[])
{
	if (argc < 1)
	{
		printf("write: error, no path given\n");
		return;
	}	
	
	char * path = argv[0];
	
	printf("writing to %s\n", path);
	
	f_entry *f = fentry_from_path(vfat, path);
	if (!f) /* if the file does not exist, create it */
	{
#if CREATE_FILE_ON_WRITE
		if (!cmd_mkfile(vfat, 1, argv)) return;
		f = fentry_from_path(vfat, path);
//		printf("f->first_cluster: %#x\n", f->first_cluster); free(f); return;
#else
		printf("file %s does not exist, use mkfile %s to create it\n", path, path);
		return;
#endif	
	} else { /* the file does exist */
		// truncate its contents
		truncate_cluster_chain(vfat, f->first_cluster, 0);
		f->first_cluster = 0;
		first_cluster_to_disk(vfat, f);
	}

	char buffer[READ_WRITE_BUFFER_SIZE];
	unsigned int total_bytes_written = 0;
	int bytes_read = fread(buffer, 1, READ_WRITE_BUFFER_SIZE, stdin);
	
	while (bytes_read)
	{	
		unsigned int bytes_written = write_file(vfat, f, buffer, bytes_read, total_bytes_written);
		total_bytes_written += bytes_read;

		//printf("writing %d bytes at %d\n", bytes_read, total_bytes_read);
		
		if (bytes_written != bytes_read) /* error writing */
		{
			printf("not all bytes were written: %d of %d\n", bytes_written, bytes_read);
			break;
		}

		bytes_read = fread(buffer, 1, READ_WRITE_BUFFER_SIZE, stdin);
	}
	
	printf("setting file size: %d\n", total_bytes_written);
	f->size_bytes = total_bytes_written;
	file_size_to_disk(vfat, f);
}

int
cmd_mv (part_info *vfat, int argc, char *argv[])
{
	if (argc < 2)
	{
		printf("mv: error, src and/or dst not given\n");
		return 0;
	}	
	
	char * src_path = argv[0];
	char * dst_path = argv[1];
	
	printf("moving %s => %s\n", src_path, dst_path);
	f_entry *f = fentry_from_path(vfat, dst_path);
	if (f)
	{
		printf("destination: %s, arlready exists\n", dst_path);
		free(f);
		return 0;
	}
	
	f = fentry_from_path(vfat, src_path);
	if (!f)
	{
		printf("source: %s, does not exist\n", src_path);
		return 0;
	}
	
	/* get the f_entry of the path leading up to the last '/' */
	char* last_slash = strrchr(dst_path, '/');
	char* dst_name = strdup((last_slash ? last_slash+1 : dst_path));
	last_slash[0] = 0; /* terminate path at last slash */
	f_entry *d = fentry_from_path(vfat, dst_path);	

	strcpy(f->lfn, dst_name);
	f->name[0] = 0;
	
//	printf("dst_dir: %s (%#x), src_file: %s (dir_cluster: %#x)\n", d->name, d->first_cluster, f->name, f->dir_first_cluster);
	
	//TODO: cmd_mv: will this work for moving a DIRECTORY? - think so?
	
	dir_rem_entry(vfat, f);
	dir_add_entry(vfat, d, f);
	
	free(dst_name);
	return 0;
}

int
cmd_mkfile (part_info *vfat, int argc, char *argv[])
{
	if (argc < 1)
	{
		printf("touch: error, no path given\n");
		return 0;
	}
	
	char * file_path = argv[0];
	printf("creating file '%s'\n", file_path);
	return entry_create(vfat, file_path, 0x0);
}

int
cmd_mkdir (part_info *vfat, int argc, char *argv[])
{
	if (argc < 1)
	{
		printf("mkdir: error, no directory given\n");
		return 0;
	}
	
	char * dir_path = argv[0];
	printf("creating directory '%s'\n", dir_path);
	//TODO: create '.' and '..' directories in new directory
	//		'.'->first_cluster = cluster of new directory
	//		'..'->first_cluster = 0 (always? - test for 3rd level directory)
	return entry_create(vfat, dir_path, 0x10); //0x10 = subdir
}

void
cmd_rmdir (part_info *vfat, int argc, char *argv[])
{
	if (argc < 1)
	{
		printf("rmdir: error, no directory given\n");
		return;
	}
	
	char * path = argv[0];
	f_entry *d = fentry_from_path(vfat, path);
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

	free(d);

	f_entry entries[1];
	
	int i=0;
	while (read_dir(vfat, path, entries, 1, i++)) /* try to read an entry from d */
	{
		if (strcmp(entries[0].name, ".") !=0 && strcmp(entries[0].name, "..") != 0)
		{
			printf("Directory not empty: %s\n", path);
			return;			
		}
	}
	
	file_rem(vfat, path);
}

void
cmd_rmfile (part_info *vfat, int argc, char *argv[])
{
	if (argc < 1)
	{
		printf("rm: error, no path given\n");
		return;
	}
	
	char * path = argv[0];
	
	f_entry *f = fentry_from_path(vfat, path);
	if (!f)
	{
		printf("%s does not exist.\n", path);
		return;
	}
	if (f->attr_d)
	{
		cmd_rmdir(vfat, 1, argv);
		return;
	}
	file_rem(vfat, path);
}

/*
 *
 * READONLY DEBUG FUNCTIONS
 *
 */

void
cmd_readcluster (part_info *vfat, int argc, char *argv[])
{
	if (argc < 1)
	{
		printf("readcluster: error, no cluster given\n");
		return;
	}
	
	char * strcluster = argv[0];
	
	char buffer[vfat->bytes_per_cluster];
	read_cluster(vfat, buffer, strtol(strcluster,0,0));
	int i, j=0;
	for (i=0;i<vfat->bytes_per_cluster;i++)
	{
		printf("%02x ", (unsigned char)buffer[i]);
		if (j++>30) {printf("\n");j=0;}
	}
	printf("\n\n");
}

void
cmd_findchainwithcluster (part_info *vfat, int argc, char *argv[])
{
	if (argc < 1)
	{
		printf("findchainwithcluster: error, no cluster given\n");
		return;
	}

	char * strcluster = argv[0];

	unsigned int search_cluster = strtol(strcluster, 0, 0);
	unsigned int cluster = 0;

	int found;
	do {
		found = 0;
		for (cluster = 2; cluster < vfat->num_data_clusters; cluster++)
		{
			unsigned int next_cluster = read_fat(vfat, cluster);
			if (next_cluster == search_cluster)
			{
				found = 1;
				search_cluster = cluster;
				break;
//				printf("invalid cluster chain, cluster#: %#x found.\n", cluster);
//				break;
			}
		}		
	} while (found);
	printf("%#x\n", search_cluster);
}


void
cmd_printclusterchain (part_info *vfat, int argc, char *argv[])
{
	if (argc < 1)
	{
		printf("printclusterchain: error, no cluster given\n");
		return;
	}
	
	char * strcluster = argv[0];
	
	unsigned int cluster = strtol(strcluster, 0, 0);
	for(;;)
	{
		if (cluster >= 0xFFFFFF8) break;
		printf("%#x ", cluster);
		if (cluster < 2)
		{
			printf("invalid cluster chain, cluster#: %#x found.\n", cluster);
			break;
		}
		cluster = read_fat(vfat, cluster);
	}
	printf("\n");
}

void
cmd_readfat (part_info *vfat, int argc, char *argv[])
{
	if (argc < 1)
	{
		printf("readfat: error, no cluster given\n");
		return;
	}

	char * strcluster = argv[0];
	printf("%#x\n", read_fat(vfat, strtol(strcluster,0,0)));
}

void
cmd_dumpdir (part_info *vfat, int argc, char *argv[])
{
	if (argc < 1)
	{
		printf("dumpdir: error, no path given\n");
		return;
	}
	
	char * path = argv[0];
	
	f_entry *f = fentry_from_path(vfat, path);
	
	if (!f)
	{
		printf("%s not found.\n", path);
		return;
	}
	
	if (!f->attr_d)
	{
		printf("%s not a directory.\n", path);
		free(f);
		return;
	}
	
	printf("first_cluster: %d, dir_slot_offset: %d, dir_num_slots: %d\n", f->first_cluster, f->dir_slot_offset, f->dir_num_slots);
	
	int bytes_read, total_read = 0;
	int slot = 0;
	do
	{
		char entry[FILE_ENTRY_SIZE];
		bytes_read = read_cluster_chain(vfat, f->first_cluster, entry, FILE_ENTRY_SIZE, total_read);
		total_read += bytes_read;
		
		if (!entry[0]) break;

		printf("%-4d: ", slot);

		int i = 0;
		if (entry[0x0b] != 0x0f)
		{
			for (i=0;i<11;i++)
			{
				printf("%2c ", entry[i]);
			}
		}
		
		for (;i<bytes_read;i++)
		{
			printf("%02x ", (unsigned char)entry[i]);
		}
		printf("\n");
		printf("\n");
		slot++;
	} while (bytes_read);
	
	free(f);
}

void
cmd_info (part_info *vfat, int argc, char *argv[])
{
	printf("OEM String: %s\n", vfat->oem);

//	printf("Bytes/Sector: %#x\n", p.bytes_per_sector);
	printf("Sectors/Cluster: %#x\n", vfat->sectors_per_cluster);
	printf("Reserved Sectors: %#x\n", vfat->reserved_sectors);
	printf("Num FAT's: %#x\n", vfat->num_fats);
//	printf("Max root entries: %#x\n", vfat->max_root_entries);
	printf("Total Sectors: %#x\n", vfat->total_sectors);
//	printf("Media: %#x\n", vfat->media);
	printf("Sectors/Fat: %#x\n", vfat->sectors_per_fat);
	printf("Root Cluster #: %#x\n", vfat->root_first_cluster);
//	printf("FS Info Sector #: %#x\n", p.fs_info_sector);
	printf("Volume Label: %s\n", vfat->label);
	printf("Free Clusters: %#x\n", vfat->free_clusters);
	printf("Partition Size: ");
	print_human_size((float)vfat->total_sectors * vfat->bytes_per_sector);
	printf(" -");
	print_human_size(vfat->reserved_sectors+(vfat->num_fats*vfat->sectors_per_fat)*vfat->bytes_per_sector);
	printf(" for filesystem\n");
	printf("%.2f%% full\n", 100.0 - 100.0 * (float)vfat->free_clusters * (float)vfat->sectors_per_cluster / (float)vfat->num_data_clusters);
}

void
cmd_todo (part_info *vfat, int argc, char *argv[])
{
	printf("commands:\n");
	printf("\tread\t[/path/to/file]\t- write contents of file to stdout\n");
	printf("\tls\t[/path/to/dir]\t- display directory listing\n");
	printf("\tinfo\t\t\t- filesystem information\n");		
	printf("\twrite\t[/path/to/file]\t- write stdin to file\n");
	printf("\trm\t[/path/to/file]\t- delete file\n");
	printf("\trmdir\t[/path/to/dir]\t- delete empty directory\n");	
	printf("\tmv\t[/old] [/new]\t- move old file to new file\n");
	printf("\ttouch\t[/new/file]\t- create file\n");
	printf("\tmkdir\t[/new/dir]\t- create directory\n");
	puts("");	
	printf("\t- entry_create\n");
	printf("\t\t- generate current timestamps\n");

	printf("\t- dir_add_entry\n");
	printf("\t\t- intelligent 8.3 generation\n");
	printf("\t\t- encode f_entry's timestamps\n");
	puts("");
	printf("\t- cache - what kind of scheme to use?\n");
//	printf("\t- cli - better parameters\n");
//	printf("\t\t ./vfat -f Win98.img -s 0x3f [cmd] \n");
}

int
main (int argc, char *argv[]) {	
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
	
	if (!path)
	{
		printf("must specify device/image path\ntry: %s -f /path/to/img todo\n", argv[0]);
		return 1;
	}
	
	FILE* f = fopen(path, "r");
	if (!f)
	{
		printf("fopen: failed opening %s\n", path);
		return 1;
	}

	part_info *vfat;
	vfat = vfat_mount(f, lba);

	if (vfat)
	{
		if (num_sub_args)
		{
			if (strcmp(sub_argv[0], "read") == 0) {
				cmd_read(vfat, num_sub_args-1, sub_argv+1);
			} else if (strcmp(sub_argv[0], "write") == 0) {
				cmd_write(vfat, num_sub_args-1, sub_argv+1);
			} else if (strcmp(sub_argv[0], "ls") == 0) {
				cmd_ls(vfat, num_sub_args-1, sub_argv+1);
			} else if (strcmp(sub_argv[0], "mv") == 0) {
				cmd_mv(vfat, num_sub_args-1, sub_argv+1);
			} else if (strcmp(sub_argv[0], "touch") == 0) {
				cmd_mkfile(vfat, num_sub_args-1, sub_argv+1);
			} else if (strcmp(sub_argv[0], "rm") == 0) {
				cmd_rmfile(vfat, num_sub_args-1, sub_argv+1);
			} else if (strcmp(sub_argv[0], "mkdir") == 0) {
				cmd_mkdir(vfat, num_sub_args-1, sub_argv+1);
			} else if (strcmp(sub_argv[0], "rmdir") == 0) {
				cmd_rmdir(vfat, num_sub_args-1, sub_argv+1);
			} else if (strcmp(sub_argv[0], "dumpdir") == 0) {
				cmd_dumpdir(vfat, num_sub_args-1, sub_argv+1);
			} else if (strcmp(sub_argv[0], "readcluster") == 0) {
				cmd_readcluster(vfat, num_sub_args-1, sub_argv+1);
			} else if (strcmp(sub_argv[0], "printclusterchain") == 0) {
				cmd_printclusterchain(vfat, num_sub_args-1, sub_argv+1);
			} else if (strcmp(sub_argv[0], "findchainwithcluster") == 0) {
				cmd_findchainwithcluster(vfat, num_sub_args-1, sub_argv+1);
			} else if (strcmp(sub_argv[0], "readfat") == 0) {
				cmd_readfat(vfat, num_sub_args-1, sub_argv+1);
			} else if (strcmp(sub_argv[0], "info") == 0) {
				cmd_info(vfat, num_sub_args-1, sub_argv+1);
			} else if (strcmp(sub_argv[0], "help") == 0 || strcmp(sub_argv[0], "todo") == 0) {
				cmd_todo(vfat, num_sub_args-1, sub_argv+1);
			//} else if (strcmp(sub_argv[0], "countnz") == 0) {
			//	count_unused_cluster_not_zerod(vfat);
		//	} else if (strcmp(cmd, "zero") == 0) {
		//		printf("this function is broken.\n");//zero_unused_clusters(vfat);
			} else {
				printf("unknown command: %s!\n", sub_argv[0]);
			}				
		} else {
			printf("no command given\n");
		}

		vfat_umount(vfat);		
	}
	
	if (sub_argv) free(sub_argv);

	fflush(f);
	fclose(f);
	
	return 0;
}
