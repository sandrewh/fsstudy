#include "cache.h"
#include <string.h>
#include <stdio.h>

uint32_t
cache_block_to_slot(CACHE* cache, uint32_t block_num)
{
	return block_num % cache->num_buckets;
}

int
cache_get(CACHE* cache, char* buffer, uint32_t block_num)
{
	if (!cache) return 0;
	
	HASH_ITEM* hash_item = hash_get(cache->hash, cache_block_to_slot(cache, block_num));
	if (!hash_item) return 0;

	CACHE_ITEM* citem = hash_item->value;
	if (citem->block_num != block_num) return 0;

//	printf("cache: get [%u] = %p\n", block, hash_item->value);
	memcpy(buffer, citem->block_data, cache->block_size);
	return 1;	
}

void
cache_set(CACHE* cache, char* buffer, uint32_t block_num)
{
	if (!cache) return;
	
	uint32_t cache_slot = cache_block_to_slot(cache, block_num);

	/* copy block data to new buffer */
	char* new_buffer = malloc(cache->block_size);
	memcpy(new_buffer, buffer, cache->block_size);
	
	/* if something else was stored at this block, free it, and take shortcut */
	HASH_ITEM* hash_item = hash_get(cache->hash, cache_slot);
	if (hash_item)
	{
		CACHE_ITEM* citem = hash_item->value;
		free(citem->block_data);
		citem->block_data = new_buffer;
		citem->block_num = block_num;
	} else {
		/* store the block data in hash */
	//	printf("cache: set [%u] = %p\n", block, new_buffer);
		CACHE_ITEM* citem = (CACHE_ITEM*)malloc(sizeof(CACHE_ITEM));
		citem->block_data = new_buffer;	
		citem->block_num = block_num;
		hash_set(cache->hash, block_num, citem);		
	}
}

CACHE*
cache_create(uint32_t block_size, uint32_t num_buckets)
{
	CACHE* cache = (CACHE*)malloc(sizeof(CACHE));
	cache->hash = hash_create(100, 0.7, 2.0);
	cache->block_size = block_size;
	cache->num_buckets = num_buckets;
	
	return cache;
}

void
cache_destroy(CACHE* cache)
{
	// hash_dump(cache->hash);
	
	int i;
	for (i=0; i<cache->hash->size; i++)
	{
		HASH_ITEM* hash_item = hash_item_at_index(cache->hash, i);
		free(hash_item->value);		
	}
	
	hash_destroy(cache->hash);
	free(cache);
}
