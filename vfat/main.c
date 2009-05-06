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

	printf("%6.1f%c", size, a);
}

void
cmd_read (part_info *p, char* path)
{
	f_entry *f = fentry_from_path(p, path);

	char buffer[READ_WRITE_BUFFER_SIZE];
	unsigned int total_bytes_read = 0;
	unsigned int bytes_read = read_file(p, f, buffer, READ_WRITE_BUFFER_SIZE, 0);
	
	while (bytes_read)
	{
		total_bytes_read += bytes_read;
	
		fwrite(buffer, bytes_read, 1, stdout);
		
		bytes_read = read_file(p, f, buffer, READ_WRITE_BUFFER_SIZE, total_bytes_read);
	}	
}

void
cmd_ls (part_info *p, char *path)
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

	f_entry entries[LS_FILES_BUFFER_SIZE];
	unsigned int total_files_read = 0;
	unsigned int files_read = read_dir(p, path, entries, LS_FILES_BUFFER_SIZE, 0);
	
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
			
			printf("%6x ", entries[i].first_cluster);

			printf("%3x ", entries[i].dir_slot_offset);

			printf("%3x ", entries[i].dir_num_slots);
			
			if (entries[i].lfn[0])
			{
				printf("%s (%s)\n", entries[i].lfn, entries[i].name);	
			} else {
				printf("%s\n", entries[i].name);				
			}
		}

		files_read = read_dir(p, path, entries, LS_FILES_BUFFER_SIZE, total_files_read);
	}
	
	printf("%d entries.\n", total_files_read);
}

/*
 *
 * WRITE FUNCTIONS
 *
 */

void
cmd_write (part_info *p, char* path)
{
	printf("writing to %s\n", path);
	
	f_entry *f = fentry_from_path(p, path);
	if (!f) /* if the file does not exist, create it */
	{
#if CREATE_FILE_ON_WRITE
		if (!cmd_mkfile(p, path)) return;
		f = fentry_from_path(p, path);
//		printf("f->first_cluster: %#x\n", f->first_cluster); free(f); return;
#else
		printf("file %s does not exist, use mkfile %s to create it\n", path, path);
		return;
#endif	
	} else { /* the file does exist */
		// truncate its contents
		truncate_cluster_chain(p, f->first_cluster, 0);
		f->first_cluster = 0;
		first_cluster_to_disk(p, f);
	}

	char buffer[READ_WRITE_BUFFER_SIZE];
	unsigned int total_bytes_written = 0;
	int bytes_read = fread(buffer, 1, READ_WRITE_BUFFER_SIZE, stdin);
	
	while (bytes_read)
	{	
		unsigned int bytes_written = write_file(p, f, buffer, bytes_read, total_bytes_written);
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
	file_size_to_disk(p, f);
}

int
cmd_mv (part_info *p, char* src_path, char* dst_path)
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
	
	/* get the f_entry of the path leading up to the last '/' */
	char* last_slash = strrchr(dst_path, '/');
	char* dst_name = strdup((last_slash ? last_slash+1 : dst_path));
	last_slash[0] = 0; /* terminate path at last slash */
	f_entry *d = fentry_from_path(p, dst_path);	

	strcpy(f->lfn, dst_name);
	f->name[0] = 0;
	
//	printf("dst_dir: %s (%#x), src_file: %s (dir_cluster: %#x)\n", d->name, d->first_cluster, f->name, f->dir_first_cluster);
	
	//TODO: cmd_mv: will this work for moving a DIRECTORY? - think so?
	
	dir_rem_entry(p, f);
	dir_add_entry(p, d, f);
	
	free(dst_name);
	return 0;
}

int
cmd_mkfile (part_info *p, char* file_path)
{
	printf("creating file '%s'\n", file_path);
	return entry_create(p, file_path, 0x0);
}

int
cmd_mkdir (part_info *p, char* dir_path)
{
	printf("creating directory '%s'\n", dir_path);
	//TODO: create '.' and '..' directories in new directory
	//		'.'->first_cluster = cluster of new directory
	//		'..'->first_cluster = 0 (always? - test for 3rd level directory)
	return entry_create(p, dir_path, 0x10); //0x10 = subdir
}

void
cmd_rmdir (part_info *p, char *path)
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

	free(d);

	f_entry entries[1];
	
	int i=0;
	while (read_dir(p, path, entries, 1, i++)) /* try to read an entry from d */
	{
		if (strcmp(entries[0].name, ".")!=0 && strcmp(entries[0].name, "..")!=0)
		{
			printf("Directory not empty: %s\n", path);
			return;			
		}
	}
	
	file_rem(p, path);
}

void
cmd_rmfile (part_info *p, char *path)
{
	f_entry *f = fentry_from_path(p, path);
	if (!f)
	{
		printf("%s does not exist.\n", path);
		return;
	}
	if (f->attr_d)
	{
		cmd_rmdir(p, path);
		return;
	}
	file_rem(p, path);
}

/*
 *
 * READONLY DEBUG FUNCTIONS
 *
 */

void
cmd_readcluster (part_info *p, char* strcluster)
{
	char buffer[p->bytes_per_cluster];
	read_cluster(p, buffer, strtol(strcluster,0,0));
	int i, j=0;
	for (i=0;i<p->bytes_per_cluster;i++)
	{
		printf("%02x ", (unsigned char)buffer[i]);
		if (j++>30) {printf("\n");j=0;}
	}
	printf("\n\n");
}

void
cmd_printclusterchain (part_info *p, char* strcluster)
{
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
		cluster = read_fat(p, cluster);
	}
	printf("\n");
}

void
cmd_readfat (part_info *p, char* strcluster)
{
	printf("%#x\n", read_fat(p, strtol(strcluster,0,0)));
}

void
cmd_dumpdir (part_info *p, char* path)
{
	f_entry *f = fentry_from_path(p, path);
	
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
		bytes_read = read_cluster_chain(p, f->first_cluster, entry, FILE_ENTRY_SIZE, total_read);
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
cmd_info (part_info *p)
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
	printf("%.2f%% full\n", 100.0 - 100.0 * (float)p->free_clusters * (float)p->sectors_per_cluster / (float)p->num_data_clusters);
}

void
cmd_todo ()
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
	printf("\t- cli - better parameters\n");
	printf("\t\t ./vfat -f Win98.img -s 0x3f [cmd] \n");
}

int
main (int argc, char *argv[]) {	
	FILE* f = fopen(argv[1], "r+");
	if (!f)
	{
		printf("fopen: failed opening %s\n", argv[1]);
		return 1;
	}

	unsigned int lba = strtol(argv[2],0,0);
//	printf("using lba: %#x\n", lba);
	part_info p;
	p = vfat_mount(f, lba);
	
	char* cmd = argv[3];
	if (strcmp(cmd, "read") == 0) {
		cmd_read(&p, argv[4]);
	} else if (strcmp(cmd, "write") == 0) {
		cmd_write(&p, argv[4]);	
	} else if (strcmp(cmd, "ls") == 0) {
		cmd_ls(&p, argv[4]);
	} else if (strcmp(cmd, "mv") == 0) {
		cmd_mv(&p, argv[4], argv[5]);
	} else if (strcmp(cmd, "touch") == 0) {
		cmd_mkfile(&p, argv[4]);		
	} else if (strcmp(cmd, "rm") == 0) {
		cmd_rmfile(&p, argv[4]);		
	} else if (strcmp(cmd, "mkdir") == 0) {
		cmd_mkdir(&p, argv[4]);
	} else if (strcmp(cmd, "rmdir") == 0) {
		cmd_rmdir(&p, argv[4]);
	} else if (strcmp(cmd, "dumpdir") == 0) {
		cmd_dumpdir(&p, argv[4]);
	} else if (strcmp(cmd, "readcluster") == 0) {
		cmd_readcluster(&p, argv[4]);		
	} else if (strcmp(cmd, "printclusterchain") == 0) {
		cmd_printclusterchain(&p, argv[4]);		
	} else if (strcmp(cmd, "readfat") == 0) {
		cmd_readfat(&p, argv[4]);				
	} else if (strcmp(cmd, "info") == 0) {
		cmd_info(&p);
	} else if (strcmp(cmd, "todo") == 0) {
		cmd_todo();
//	} else if (strcmp(cmd, "countnz") == 0) {
//		count_unused_cluster_not_zerod(&p);
//	} else if (strcmp(cmd, "zero") == 0) {
//		printf("this function is broken.\n");//zero_unused_clusters(&p);
	} else {
		printf("unknown command: %s!\n", cmd);
	}

	vfat_umount(&p);
	fflush(f);
	fclose(f);
	
	return 0;
}
