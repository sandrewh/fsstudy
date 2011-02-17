#ifndef CACHE_H
#define CACHE_H

#include "../util/hash.h"

typedef struct {
	HASH* hash;
	uint32_t block_size;
	uint32_t num_buckets;
} CACHE;

typedef struct {
	uint32_t block_num;
	uint8_t* block_data;
} CACHE_ITEM;

void cache_info(CACHE* cache);
int cache_get(CACHE* cache, uint8_t* buffer, uint32_t block);
void cache_set(CACHE* cache, uint8_t* buffer, uint32_t block);
CACHE* cache_create(uint32_t block_size, uint32_t num_buckets);
void cache_destroy(CACHE* cache);

#endif