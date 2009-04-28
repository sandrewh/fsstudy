#ifndef NTFS_H
#define NTFS_H

#include <stdio.h>

#include "ntfs_info.h"

ntfs_info* ntfs_mount(FILE* f, unsigned int partition_start_sector);
void ntfs_umount(ntfs_info *ntfs);

#endif
