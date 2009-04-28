#ifndef CACHE_H
#define CACHE_H

#include <stdio.h>
#include "part_info.h"

void raw_write_sector(part_info *p, char* buffer, unsigned int sector);
void raw_read_sector(part_info *p, char* buffer, unsigned int sector);
void raw_write_fat(part_info *p, unsigned int cluster, unsigned int value);
unsigned int raw_read_fat(part_info *p, unsigned int cluster);
void cache_init(part_info *p);
void cache_write_sector(part_info *p, char *buffer, unsigned int sector);
void cache_read_sector(part_info *p, char *buffer, unsigned int sector);
void cache_write_fat(part_info *p, unsigned int cluster, unsigned int value);
unsigned int cache_read_fat(part_info *p, unsigned int cluster);
void cache_flush(part_info *p);
unsigned int read_fat(part_info *p, unsigned int cluster);
void write_fat(part_info *p, unsigned int cluster, unsigned int value);

#endif
