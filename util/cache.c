#include "cache.h"
#include <string.h>
#include <stdio.h>

uint32_t
cache_block_to_slot(CACHE* cache, uint32_t block_num)
{
	return block_num % cache->num_buckets;
}

int cache_misses = 0;
int cache_hits = 0;
int cache_overwrites = 0;
int cache_sets = 0;

void
cache_info(CACHE* cache)
{
	printf("slots: %d, cap: %d K, get: %d, miss: %d, hit: %d, hit-rate: %.4f, set: %d, over: %d\n", cache->num_buckets, cache->num_buckets*cache->block_size/1024, cache_misses+cache_hits, cache_misses, cache_hits, (float)cache_hits/(float)(cache_hits+cache_misses), cache_sets, cache_overwrites);
}

int
cache_get(CACHE* cache, uint8_t* buffer, uint32_t block_num)
{
	if (!cache) return 0;

	HASH_ITEM* hash_item = hash_get(cache->hash, cache_block_to_slot(cache, block_num));
	if (!hash_item) return 0;

	CACHE_ITEM* citem = hash_item->value;
	if (citem->block_num != block_num)
	{
		cache_misses++;
		return 0;
	}
	cache_hits++;
	
	// printf("cache: get [0x%x] = %p\n", block_num, hash_item->value);
	memcpy(buffer, citem->block_data, cache->block_size);
	return 1;
}

void
cache_set(CACHE* cache, uint8_t* buffer, uint32_t block_num)
{
	if (!cache) return;
	
	cache_sets++;
	
	uint32_t cache_slot = cache_block_to_slot(cache, block_num);

	/* copy block data to new buffer */
	uint8_t* new_buffer = malloc(cache->block_size);
	memcpy(new_buffer, buffer, cache->block_size);
	
	/* if something else was stored at this block, free it, and take shortcut */
	HASH_ITEM* hash_item = hash_get(cache->hash, cache_slot);
	if (hash_item)
	{
		CACHE_ITEM* citem = hash_item->value;
		free(citem->block_data);
		citem->block_data = new_buffer;
		citem->block_num = block_num;
		cache_overwrites++;
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
	cache->hash = hash_create(num_buckets, 0.7, 2.0);
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
