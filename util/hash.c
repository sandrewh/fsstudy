#include "hash.h"
#include <stdio.h>

/* create hastable, iniitialize, and return  hashtable */
HASH* hash_create(int capacity, float threshold, float multiplier) {
	/* get space for hash, initialize it */
	HASH* hash = (HASH*)malloc(sizeof(HASH));
	hash->size = 0;
	hash->capacity = capacity;
	hash->threshold = threshold;
	hash->multiplier = multiplier;

	/* get space for hash_items, initialize them 
	   all to 0 (empty) */
	hash->items = (HASH_ITEM**)calloc(capacity, sizeof(HASH_ITEM*));
	int i;
	for (i = 0; i < capacity; ++i) {
		hash->items[i] = (HASH_ITEM*)0;
	}

	return hash;
}

/* given a key, return the table index */
int hash_key_to_index(HASH* hash, uint32_t key) {
	/* key is an int; index = key mod capacity */
	return key % hash->capacity;
}

/* expand table size and rehash all elemeents */
void hash_rehash(HASH* hash) {
	/* expands hash and re-inserts items  */

	int new_capacity = hash->capacity * hash->multiplier;
	HASH* new_hash = hash_create(new_capacity, hash->threshold, hash->multiplier);
	
	int i;
	for (i = 0; i < hash->capacity; ++i) {
		HASH_ITEM* item = hash->items[i];
		while (item) {
			HASH_ITEM* next_item = item->next;
			hash_set(new_hash, item->key, item->value);
			free(item);
			item = next_item;
		}
	}

	free (hash->items);
	hash->items = new_hash->items;
	hash->capacity = new_capacity;
	free (new_hash);
}

/* returns hash_item w/ key=key,
   or the closest one */ 
HASH_ITEM* hash_get_close(HASH* hash, uint32_t key) {

	int index = hash_key_to_index(hash, key);	
	HASH_ITEM* item = hash->items[index];

	if (!item) {
		return (HASH_ITEM*)0;
	}

	while (item->next) {
		if (item->key == key) {
			return item;
		}

		item = item->next;
	}
	
	return item;
}

/* sets or adds key=>value to table */
void hash_set(HASH* hash, uint32_t key, VALUE_TYPE value) {
	/* find key item or new location */
	HASH_ITEM* item;
	if ((item = hash_get_close(hash, key))) {
		if (item->key == key) {
			/* key already exists */
			item->value = value;
			return;
		}
	}
	
	/* key doesn't exist */
	hash->size++;
	
	/* create a new hash item */
	HASH_ITEM* new_item = (HASH_ITEM*)malloc(sizeof(HASH_ITEM));
	new_item->key = key;
	new_item->value = value;
	new_item->prev = (HASH_ITEM*)0;	
	new_item->next = (HASH_ITEM*)0;

	/* add the item to the hash */
	if (item) {
		/* item points to last item in linked-list */
		item->next = new_item;
		new_item->prev = item;
	} else {
		int index = hash_key_to_index(hash, key);	
		hash->items[index] = new_item;
	}

	/* time to rehash? */	
	if ((float)hash->size / (float)hash->capacity > hash->threshold) {
		hash_rehash(hash);
	}
}

void
hash_destroy(HASH* hash) {
	int i;
	for (i = 0; i < hash->capacity; ++i) {
		HASH_ITEM* item = hash->items[i];
		while (item) {
			HASH_ITEM* next_item = item->next;
			free(item);
			item = next_item;
		}
	}
	
	free (hash->items);
	free (hash);
}

HASH_ITEM*
hash_item_at_index(HASH* hash, uint32_t index)
{
	int i, j=0;
	for (i = 0; i < hash->capacity; ++i) {
		HASH_ITEM* item = hash->items[i];
		while (item) {
			if (index == j) return item;
			j++;
			item = item->next;
		}
	}
	
	return NULL;
}

/* returns hash element w/ key = key
	or NULL (0) */
HASH_ITEM* hash_get(HASH* hash, uint32_t key) {
	HASH_ITEM* item = hash_get_close(hash, key);

	if (!item || item->key != key) {
		return (HASH_ITEM*)0;
	}
	
	return item;
}

void hash_delete(HASH* hash, uint32_t key) {
	HASH_ITEM* item = hash_get(hash, key);
	if (!item) return;
	item->prev->next = item->next;
	item->next->prev = item->prev;
	free(item);
}

/* dumps the contents of the table to STDOUT */
void hash_dump(HASH* hash) {
	printf("hash:\n");
	
	int i;
	for (i = 0; i < hash->capacity; ++i) {
		printf("%d:\t", i);
		HASH_ITEM* item = hash->items[i];
		while (item) {
			printf("[%d:%p] ", item->key, item->value);
			item = item->next;
		}
		printf("\n");
	}

	printf("size: %d, capacity: %d, threshold: %4f, multiplier: %4f\n\n",
		hash->size, hash->capacity, hash->threshold, hash->multiplier);
}