#ifndef HFSP_H
#define HFSP_H

#include "hfsp_info.h"

hfsp_info* hfsp_mount(FILE* f, unsigned int partition_start_sector);
void hfsp_umount(hfsp_info *hfsp);

#endif