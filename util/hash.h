#ifndef HASH_H
#define HASH_H

#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>

#define VALUE_TYPE void*

struct HASH_ITEM {
	uint32_t key;
	VALUE_TYPE value;
	struct HASH_ITEM* prev;
	struct HASH_ITEM* next;
};

typedef struct HASH_ITEM HASH_ITEM;

typedef struct {
	int size;
	int capacity;
	float threshold;
	float multiplier;
	HASH_ITEM** items;
} HASH;

HASH* hash_create(int capacity, float threshold, float multiplier);
HASH_ITEM* hash_item_at_index(HASH* hash, uint32_t index);
int hash_key_to_index(HASH* hash, uint32_t key);
void hash_set(HASH* hash, uint32_t key, VALUE_TYPE value);
void hash_destroy(HASH* hash);
HASH_ITEM* hash_get_close(HASH* hash, uint32_t key);
HASH_ITEM* hash_get(HASH* hash, uint32_t key);
void hash_delete(HASH* hash, uint32_t key);
void hash_dump(HASH* hash);
#endif