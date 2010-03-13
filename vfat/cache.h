#ifndef CACHE_H
#define CACHE_H

#include <stdio.h>
#include "part_info.h"

void raw_write_sector(part_info *p, char* buffer, uint32_t sector);
void raw_read_sector(part_info *p, char* buffer, uint32_t sector);
void raw_write_fat(part_info *p, uint32_t cluster, uint32_t value);
uint32_t raw_read_fat(part_info *p, uint32_t cluster);
void cache_init(part_info *p);
void cache_write_sector(part_info *p, char *buffer, uint32_t sector);
void cache_read_sector(part_info *p, char *buffer, uint32_t sector);
void cache_write_fat(part_info *p, uint32_t cluster, uint32_t value);
uint32_t cache_read_fat(part_info *p, uint32_t cluster);
void cache_flush(part_info *p);
uint32_t read_fat(part_info *p, uint32_t cluster);
void write_fat(part_info *p, uint32_t cluster, uint32_t value);

#endif
