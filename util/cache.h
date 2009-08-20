#ifndef CACHE_H
#define CACHE_H

#include "../util/hash.h"

typedef struct {
	HASH* hash;
	uint32_t block_size;
} CACHE;

int cache_get(CACHE* cache, char* buffer, uint32_t block);
void cache_set(CACHE* cache, char* buffer, uint32_t block);
CACHE* cache_create(uint32_t block_size);
void cache_destroy(CACHE* cache);

#endif