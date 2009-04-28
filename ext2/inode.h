#ifndef INODE_H
#define INODE_H

#include "ext2_info.h"

void inode_read(ext2_info *ext2, inode *in, unsigned int inode_num);

#endif