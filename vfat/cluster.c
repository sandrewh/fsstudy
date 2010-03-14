#include <string.h>

#include "cluster.h"
#include "sector.h"

/*
 *
 * READ ONLY FUNCTIONS
 *
 */

void
read_cluster (part_info *p, char* buffer, uint32_t cluster)
{
	unsigned int sector_num;
	for (sector_num=0; sector_num<p->sectors_per_cluster;sector_num++)
		read_sector(p, buffer+(sector_num*p->bytes_per_sector), p->clusters_first_sector + (cluster - 2) * p->sectors_per_cluster + sector_num);
}

unsigned int
read_cluster_chain (part_info *p, uint32_t first_cluster, char* buffer, size_t size, off_t offset)
{
	uint32_t cur_cluster = first_cluster;		
	if (!cur_cluster) return 0; /* no chain provided */

	uint32_t offset_cluster_num = offset / p->bytes_per_cluster;

	/* skip to the cluster with our offset */
	uint32_t cluster_num;
	for (cluster_num=0;cluster_num<offset_cluster_num;cluster_num++)
	{
			cur_cluster = read_fat(p, cur_cluster);	
			if (cur_cluster >= 0xFFFFFF8) return 0; /* chain ended before offset */
	}
	
	char cluster_buffer[p->bytes_per_cluster];
	uint32_t cluster_byte_offset = offset % p->bytes_per_cluster;
//	printf("starting at byte %d of cluster %d (%d)\n\n", cluster_byte_offset, offset_cluster_num, cur_cluster);

	uint32_t bytes_read = 0;
	while (bytes_read < size)
	{
		read_cluster(p, cluster_buffer, cur_cluster);

		uint32_t bytes_to_read = p->bytes_per_cluster - cluster_byte_offset;
		if (size - bytes_read < bytes_to_read) bytes_to_read = size - bytes_read;

//		printf("%d bytes from cluster %d::%d to buffer[%d]\n", bytes_to_read, cur_cluster, cluster_byte_offset, bytes_read);

		memcpy(buffer+bytes_read, cluster_buffer+cluster_byte_offset, bytes_to_read);
		cluster_byte_offset = 0;
		bytes_read += bytes_to_read;
		cur_cluster = read_fat(p, cur_cluster);
		if (cur_cluster >= 0xFFFFFF8) break; /* chain ended */
	}

	return size;	
}

/*
 *
 * WRITE ONLY FUNCTIONS
 *
 */

uint32_t
find_free_cluster (part_info *p, uint32_t hint)
{
	/* TODO: find_free_cluster: use random search? */
	if (!hint) hint = 2;
	
	/* search through fat until free cluster is found */
	uint32_t i;
	for (i = hint; i < p->num_data_clusters+2; i++)
	{
		if (!read_fat(p, i)) return i;
	}
	
	// TODO: if we had a hint, but couldn't find a free cluster, start at 0 and search to hint-1

	return 0; /* disk full */
}

void
write_cluster (part_info *p, char* buffer, uint32_t cluster)
{
	uint32_t sector_num;
	for (sector_num=0; sector_num<p->sectors_per_cluster;sector_num++)
		write_sector(p, buffer+(sector_num*p->bytes_per_sector), p->clusters_first_sector + (cluster - 2) * p->sectors_per_cluster + sector_num);
}

//TODO: this needs to be optimized for successive calls (N^2 hurts for large files)
//		perhaps provide a hint? - not sure
uint32_t
write_cluster_chain (part_info *p, uint32_t first_cluster, char* buffer, size_t size, off_t offset)
{
//	printf("write_cluster_chain: first_cluster: %#x, size: %#x, offset: %#x\n", first_cluster, size, offset);
	uint32_t cur_cluster = first_cluster;		
	if (!cur_cluster) return 0; /* no chain provided */

	uint32_t offset_cluster_num = offset / p->bytes_per_cluster;

	/* skip to the cluster with our offset, extending chain if necessary */
	uint32_t cluster_num;
	for (cluster_num=0;cluster_num<offset_cluster_num;cluster_num++)
	{
//			printf("write_cluster_chain: skipping cluster %#x (+%#x)\n", cur_cluster, cluster_num);
			uint32_t next_cluster = read_fat(p, cur_cluster);	
			if (next_cluster >= 0xFFFFFF8)
			{
				/* chain ended before offset */
				/* extend chain */
				next_cluster = allocate_new_cluster(p, cur_cluster+1);
				write_fat(p, cur_cluster, next_cluster);
//				printf("write_cluster_chain: end of chain @ cluster %#x (+%#x), appended cluster %#x\n", cur_cluster, cluster_num, next_cluster);
			}
			cur_cluster = next_cluster;
	}

	char cluster_buffer[p->bytes_per_cluster];
	uint32_t cluster_byte_offset = offset % p->bytes_per_cluster;
//	printf("write_cluster_chain: starting at byte %#x of cluster %#x (+%#x)\n", cluster_byte_offset, cur_cluster, offset_cluster_num);

	uint32_t bytes_written = 0;
	for (;;)
	//while (bytes_written < size)
	{		
		uint32_t bytes_to_write = p->bytes_per_cluster - cluster_byte_offset;
		if (size - bytes_written < bytes_to_write) bytes_to_write = size - bytes_written;

//		printf("write_cluster_chain: %#x bytes from buffer[%#x] to cluster %#x::%#x\n", bytes_to_write, bytes_written, cur_cluster, cluster_byte_offset);
		//if (!bytes_to_write) break;
		
		read_cluster(p, cluster_buffer, cur_cluster);
		memcpy(cluster_buffer+cluster_byte_offset, buffer+bytes_written, bytes_to_write);
		write_cluster(p, cluster_buffer, cur_cluster);		

		cluster_byte_offset = 0;
		bytes_written += bytes_to_write;

		if (bytes_written >= size) break;
		uint32_t next_cluster = read_fat(p, cur_cluster);	
		if (next_cluster >= 0xFFFFFF8)
		{
			/* chain ended before offset */
			/* extend chain */
			next_cluster = allocate_new_cluster(p, cur_cluster+1);
			write_fat(p, cur_cluster, next_cluster);
//			printf("write_cluster_chain: end of chain @ cluster %#x, appended cluster %#x\n", cur_cluster, next_cluster);			
		}
		cur_cluster = next_cluster;
	}
	
//	printf("\n");

	return size;
}


uint32_t
allocate_new_cluster (part_info *p, uint32_t hint)
{
	uint32_t new_cluster = find_free_cluster(p, hint);
	if (!new_cluster)
	{
		printf("dir_add_entry: no free cluster found, entry not added");
		return 0;
	}

	/* mark the new cluster as used (last in chain) */
	write_fat(p, new_cluster, 0x0FFFFFFF);
	return new_cluster;
}

// TODO: truncate_cluster_chain: test w/ len > 0
// should do the following:
// if N is the number of clusters in the chain, 
// free's N-len clusters from end of chain
void
truncate_cluster_chain (part_info *p, uint32_t first_cluster, size_t len)
{
//	printf("truncate_cluster_chain: first_cluster: %#x, len: %#x\n", first_cluster, len);
	
	if (len)
	{
		printf("truncate_cluster_chain: warning - not tested when len > 0 (or == 0)\n");
	}
	
	if (!first_cluster)
	{
		//printf("truncate_cluster_chain: not a valid cluster chain\n");
		return; /* not a valid cluster */
	}
	
	uint32_t cur_cluster = first_cluster;		
	uint32_t next_cluster = read_fat(p, cur_cluster);
	
	size_t cluster_num = 0;

	/* loop over clusters in chain */
	while (next_cluster < 0x0FFFFFF8)
	{
		next_cluster = read_fat(p, cur_cluster);
		
		cluster_num++;
		if (cluster_num < len) continue;
		if (cluster_num == len) write_fat(p, cur_cluster, 0x0FFFFFFF);

		write_fat(p, cur_cluster, 0x0);
		cur_cluster = next_cluster;		
	}
}
