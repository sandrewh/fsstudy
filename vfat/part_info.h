#ifndef PART_INFO_H
#define PART_INFO_H

#include <stdio.h>

typedef struct {
	char oem[9], label[12];
	unsigned short bytes_per_sector;
	unsigned char sectors_per_cluster;
	unsigned short reserved_sectors;
	unsigned char num_fats;
	unsigned short max_root_entries;
	unsigned int total_sectors;
	unsigned char media;
	unsigned int sectors_per_fat;
	unsigned short fs_info_sector;
	unsigned int first_sector;

	unsigned int fat_first_sector;
	unsigned int clusters_first_sector;
	unsigned int root_first_cluster;
	unsigned int free_clusters;
	unsigned int free_clusters_on_mount;
	unsigned int bytes_per_cluster;
	
	unsigned int num_data_clusters;
	unsigned int last_allocated_cluster;
	unsigned int last_allocated_cluster_on_mount;
	
	FILE* f;
} part_info;

#endif