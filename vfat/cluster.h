#ifndef CLUSTER_H
#define CLUSTER_H

#include "part_info.h"

unsigned int allocate_new_cluster(part_info *p, unsigned int hint);
void write_sector(part_info *p, char* buffer, unsigned int sector);
void read_sector(part_info *p, char* buffer, unsigned int sector);
void write_cluster(part_info *p, char* buffer, unsigned int cluster);
void read_cluster(part_info *p, char* buffer, unsigned int cluster);
unsigned int find_free_cluster(part_info *p, unsigned int hint);
unsigned int read_cluster_chain(part_info *p, unsigned int first_cluster, char* buffer, size_t size, off_t offset);
unsigned int write_cluster_chain(part_info *p, unsigned int first_cluster, char* buffer, size_t size, off_t offset);
void truncate_cluster_chain(part_info *p, unsigned int first_cluster, size_t len);

#endif