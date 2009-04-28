#include "vfat.h"
#include "cache.h" //for debugging functions: read_fat, etc.

void print_human_size(float size);
void cmd_read(part_info *p, char* path);
void cmd_ls(part_info *p, char *path);
void cmd_write(part_info *p, char* path);
int cmd_mv(part_info *p, char* src_path, char* dst_path);
int cmd_mkfile(part_info *p, char* file_path);
int cmd_mkdir(part_info *p, char* dir_path);
void cmd_rmdir(part_info *p, char *path);
void cmd_rmfile(part_info *p, char *path);
void cmd_readcluster(part_info *p, char* strcluster);
void cmd_printclusterchain(part_info *p, char* strcluster);
void cmd_readfat(part_info *p, char* strcluster);
void cmd_dumpdir(part_info *p, char* path);
void cmd_info(part_info *p);
void cmd_todo();
