#include "block.h"
#include "../util/cache.h"

unsigned int
block_read (ext2_info *ext2, char* buffer, unsigned int block_num)
{
	size_t ret;
	
	if (!cache_get(ext2->cache, buffer, block_num))
	{
//		printf("block_read: block_num=0x%x\n", block_num);

		/* seek to the location of the block */
		if (fseek(ext2->f, ext2->volume_offset + block_num*ext2->bytes_per_block, SEEK_SET))
			return 0;
		/* read the block to [buffer]
			fread return # of items read (0 or 1) */
		ret = fread(buffer, ext2->bytes_per_block, 1, ext2->f) * ext2->bytes_per_block;	
		
		cache_set(ext2->cache, buffer, block_num);
	} else {
		ret = ext2->bytes_per_block;
	}
	
	return ret;
}

unsigned int
super_block_read (ext2_info *ext2)
{
	/* OPT: check super_block magic number
		if bad, try other superblocks */
	
	/* read the first super block */
	if (fseek(ext2->f, ext2->volume_offset + ext2->super_block_offset, SEEK_SET))
		return 0;
	int ret = fread(&ext2->s, sizeof(ext2->s), 1, ext2->f);

	ext2->bytes_per_block = 1024 << ext2->s.s_log_block_size;
	
	return ret;
}

void
bg_desc_read (ext2_info *ext2, bg_desc *bg, unsigned int bg_num)
{
	/* OPT: should read bg_desc from a "closer" bg
		for now, just read the first copy */

	/* read the bg desc */
	/* block_num = 2 for block_size: 1KB, or 1 if block_size > 1KB */
	unsigned int block_num = (ext2->super_block_offset + ext2->bytes_per_block)/ext2->bytes_per_block;
	char block[ext2->bytes_per_block];
	block_read(ext2, block, block_num);
	
	*bg = *((bg_desc*)block + bg_num);
}

#if 0
unsigned int
block_write (ext2_info *ext2, char* buffer, unsigned int block_num)
{
	fseek(ext2->f, ext2->volume_offset + block_num*ext2->bytes_per_block, SEEK_SET);
	/* fread return # of items read (0 or 1) */
	return fwrite(buffer, ext2->bytes_per_block, 1, ext2->f) * ext2->bytes_per_block;	
}
#endif