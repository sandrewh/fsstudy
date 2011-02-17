#ifndef CLUSTER_H
#define CLUSTER_H

#include "part_info.h"

void read_cluster (part_info *p, uint8_t* buffer, uint32_t cluster);
unsigned int read_cluster_chain (part_info *p, uint32_t first_cluster, uint8_t* buffer, size_t size, off_t offset);
uint32_t find_free_cluster (part_info *p, uint32_t hint);
void write_cluster (part_info *p, uint8_t* buffer, uint32_t cluster);
uint32_t write_cluster_chain (part_info *p, uint32_t first_cluster, uint8_t* buffer, size_t size, off_t offset);
uint32_t allocate_new_cluster (part_info *p, uint32_t hint);
void truncate_cluster_chain (part_info *p, uint32_t first_cluster, size_t len);

#endif