#include "cache.h"
#include <stdlib.h>

/* initialize the cache */
void
cache_init (part_info *p)
{
	/* load fat into cache */
	p->cache_fat = malloc(p->sectors_per_fat * p->bytes_per_sector);
	unsigned int sector;
	for (sector = 0; sector < p->sectors_per_fat; sector++)
	{
		raw_read_sector(p, p->cache_fat + sector*(p->bytes_per_sector), p->first_sector + p->fat_first_sector + sector);
	}
}

/* write "dirty"? cache to disk */
/* free cache memory */
void
cache_flush (part_info *p)
{
	free(p->cache_fat);
	p->cache_fat = NULL;
	
	printf("WARNING: cache is not flushed to disk!\n");
}

/*
 *
 * READ ONLY FUNCTIONS
 *
 */

// TODO: add caching of sectors

void
cache_read_sector (part_info *p, char *buffer, uint32_t sector)
{
	printf("WARNING: cache_read_sector: not implemented!\n");		
}

uint32_t
cache_read_fat (part_info *p, uint32_t cluster)
{
	// printf("cache_read_fat {cluster: %x}\n", cluster);
	uint32_t ret = *(uint32_t*) &(p->cache_fat[cluster * sizeof(uint32_t)]);
	return ret & 0x0FFFFFFF;
}

/*
 *
 * WRITE ONLY FUNCTIONS
 *
 */

void
cache_write_sector (part_info *p, char *buffer, uint32_t sector)
{
	printf("WARNING: cache_write_sector: not implemented!\n");	
}

void
cache_write_fat (part_info *p, unsigned int cluster, uint32_t value)
{
	printf("WARNING: cache_write_fat: not implemented!\n");
}
