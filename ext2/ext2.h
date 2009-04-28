#ifndef EXT2_H
#define EXT2_H

#include "ext2_info.h"

ext2_info* ext2_mount(FILE* f, unsigned int partition_start_sector);
void ext2_umount(ext2_info *ext2);
directory_entry* path_to_entry(ext2_info *ext2, char *path);
int read_file(ext2_info *ext2, directory_entry *d, char* buffer, size_t size, off_t offset);
unsigned int read_dir(ext2_info *ext2, inode *in, directory_entry *entries, size_t size, off_t offset);

#endif