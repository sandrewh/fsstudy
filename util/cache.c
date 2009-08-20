#include "cache.h"
#include <string.h>
#include <stdio.h>

int
cache_get(CACHE* cache, char* buffer, uint32_t block)
{
	if (!cache) return 0;
	
	HASH_ITEM* hash_item = hash_get(cache->hash, block);
	if (!hash_item) return 0;
	
//	printf("cache: get [%u] = %p\n", block, hash_item->value);
	memcpy(buffer, hash_item->value, cache->block_size);
	return 1;	
}

void
cache_set(CACHE* cache, char* buffer, uint32_t block)
{
	if (!cache) return;
	
	char* new_buffer = malloc(cache->block_size);
	memcpy(new_buffer, buffer, cache->block_size);
	
//	printf("cache: set [%u] = %p\n", block, new_buffer);
	hash_set(cache->hash, block, new_buffer);
}

CACHE*
cache_create(uint32_t block_size)
{
	CACHE* cache = (CACHE*)malloc(sizeof(CACHE));
	cache->hash = hash_create(100, 0.7, 2.0);
	cache->block_size = block_size;
	
	return cache;
}

void
cache_destroy(CACHE* cache)
{
//	hash_dump(cache->hash);
	
	int i;
	for (i=0; i<cache->hash->size; i++)
	{
		HASH_ITEM* hash_item = hash_item_at_index(cache->hash, i);
		free(hash_item->value);		
	}
	
	hash_destroy(cache->hash);
	free(cache);
}
