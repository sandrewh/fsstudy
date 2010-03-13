#ifndef CLUSTER_H
#define CLUSTER_H

#include "part_info.h"

uint32_t allocate_new_cluster(part_info *p, uint32_t hint);
void write_sector(part_info *p, char* buffer, uint32_t sector);
void read_sector(part_info *p, char* buffer, uint32_t sector);
void write_cluster(part_info *p, char* buffer, uint32_t cluster);
void read_cluster(part_info *p, char* buffer, uint32_t cluster);
uint32_t find_free_cluster(part_info *p, uint32_t hint);
uint32_t read_cluster_chain(part_info *p, uint32_t first_cluster, char* buffer, size_t size, off_t offset);
uint32_t write_cluster_chain(part_info *p, uint32_t first_cluster, char* buffer, size_t size, off_t offset);
void truncate_cluster_chain(part_info *p, uint32_t first_cluster, size_t len);

#endif