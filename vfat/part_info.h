#ifndef PART_INFO_H
#define PART_INFO_H

#include <stdio.h>
#include <stdint.h>

#include "../util/cache.h"

typedef struct {
	uint8_t oem[9], label[12];
	uint16_t bytes_per_sector;
	uint8_t sectors_per_cluster;
	uint16_t reserved_sectors;
	uint8_t num_fats;
	uint16_t max_root_entries;
	uint32_t total_sectors;
	uint8_t media;
	uint32_t sectors_per_fat;
	uint16_t fs_info_sector;
	uint32_t first_sector;

	uint32_t fat_first_sector;
	uint32_t clusters_first_sector;
	uint32_t root_first_cluster;
	uint32_t free_clusters;
	uint32_t free_clusters_on_mount;
	uint32_t bytes_per_cluster;
	
	uint32_t num_data_clusters;
	uint32_t last_allocated_cluster;
	uint32_t last_allocated_cluster_on_mount;
	
	uint8_t * fat_cache;
	CACHE* sector_cache;
	
	FILE* f;
} part_info;

#endif