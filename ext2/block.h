#ifndef BLOCK_H
#define BLOCK_H

#include "ext2_info.h"

unsigned int block_read(ext2_info *ext2, char* buffer, unsigned int block_num);
unsigned int super_block_read(ext2_info *ext2);
unsigned int block_write(ext2_info *ext2, char* buffer, unsigned int block_num);
void bg_desc_read(ext2_info *ext2, bg_desc *bg, unsigned int bg_num);

#endif