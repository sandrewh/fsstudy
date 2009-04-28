#include "cache.h"

/*
 *
 * READ ONLY FUNCTIONS
 *
 */

unsigned int read_fat(part_info *p, unsigned int cluster)
{
	/* only use 1st fat, for now */
	// TODO: why use other fats? */
#if USE_CACHE
	return cache_read_fat(p, cluster);
#else
	return raw_read_fat(p, cluster);
#endif
}

void raw_read_sector(part_info *p, char* buffer, unsigned int sector)
{
//	printf("read_sector: %#x\n", sector);
	fseek(p->f, (p->first_sector + sector) * p->bytes_per_sector, SEEK_SET);	
	fread(buffer, 1, p->bytes_per_sector, p->f);
}

unsigned int raw_read_fat(part_info *p, unsigned int cluster)
{
	unsigned int ret;
	fseek(p->f, (p->first_sector + p->fat_first_sector) * p->bytes_per_sector + cluster * 4, SEEK_SET);	
	fread(&ret, 1, 4, p->f);
	ret &= 0xFFFFFFF;
	return ret;	
}

// TODO: add caching of sectors

/* initialize the cache */
void cache_init(part_info *p)
{
	
}

void cache_read_sector(part_info *p, char *buffer, unsigned int sector)
{
	
}

unsigned int cache_read_fat(part_info *p, unsigned int cluster)
{
	return 0;
}

/* write "dirty"? cache to disk */
/* free cache memory */
void cache_flush(part_info *p)
{
	
}

/*
 *
 * WRITE ONLY FUNCTIONS
 *
 */

void raw_write_sector(part_info *p, char* buffer, unsigned int sector)
{
//	printf("write_sector: %#x\n", sector);
	fseek(p->f, (p->first_sector + sector) * p->bytes_per_sector, SEEK_SET);	
	fwrite(buffer, 1, p->bytes_per_sector, p->f);
}

void write_fat(part_info *p, unsigned int cluster, unsigned int value)
{
	/* write entry to all fats */
	value &= 0xFFFFFFF; /* neccessary? */
	
#if USE_CACHE
	cache_write_fat(p, cluster, value);
#else
	raw_write_fat(p, cluster, value);
#endif
}

void raw_write_fat(part_info *p, unsigned int cluster, unsigned int value)
{
	// update p->free_clusters, if neccesary
	unsigned int old_value = read_fat(p, cluster);
	if (!old_value && value)
	{
		p->free_clusters--;
		p->last_allocated_cluster = cluster;
	} else if (old_value && !value) p->free_clusters++;

	unsigned int fat_num;
	for (fat_num=0;fat_num<p->num_fats;fat_num++)
	{
		fseek(p->f, (p->first_sector + p->fat_first_sector + fat_num * p->sectors_per_fat) * p->bytes_per_sector + cluster * 4, SEEK_SET);	
		fwrite(&value, 1, 4, p->f);
	}	
}

void cache_write_sector(part_info *p, char *buffer, unsigned int sector)
{
	
}
void cache_write_fat(part_info *p, unsigned int cluster, unsigned int value)
{
	
}
