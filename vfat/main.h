#include "vfat.h"

void print_human_size(float size);

void cmd_read (part_info *vfat, int argc, char *argv[]);
void cmd_ls (part_info *vfat, int argc, char *argv[]);
void cmd_write (part_info *vfat, int argc, char *argv[]);
int cmd_mv (part_info *vfat, int argc, char *argv[]);
int cmd_mkfile (part_info *vfat, int argc, char *argv[]);
int cmd_mkdir (part_info *vfat, int argc, char *argv[]);
void cmd_rmdir (part_info *vfat, int argc, char *argv[]);
void cmd_rmfile (part_info *vfat, int argc, char *argv[]);
void cmd_readcluster (part_info *vfat, int argc, char *argv[]);
void cmd_printclusterchain (part_info *vfat, int argc, char *argv[]);
void cmd_readfat (part_info *vfat, int argc, char *argv[]);
void cmd_dumpdir (part_info *vfat, int argc, char *argv[]);
void cmd_info (part_info *vfat, int argc, char *argv[]);
void cmd_todo (part_info *vfat, int argc, char *argv[]);
