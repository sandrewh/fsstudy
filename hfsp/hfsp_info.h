#ifndef HFSP_INFO_H
#define HFSP_INFO_H

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

typedef struct 
{
	unsigned int volume_offset;
	unsigned int super_block_offset;
	unsigned int bytes_per_block;
	FILE* f;
} hfsp_info;

#endif