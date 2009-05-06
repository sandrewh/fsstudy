#include "inode.h"
#include "block.h"

void
inode_read (ext2_info *ext2, inode *in, unsigned int inode_num)
{
	/* determine the block group that contains the inode table that contains the inode */
	unsigned int block_group_num = (inode_num - 1) / ext2->s.s_inodes_per_group;
	/* determine the offset in the inode table of the inode */
	unsigned int local_inode_index = (inode_num - 1) % ext2->s.s_inodes_per_group;

	/* get the bg_desc of the block group that contains the inode table */
	bg_desc bg_d;
	bg_desc_read(ext2, &bg_d, block_group_num);

	/* the block offset of the inode table that contains the inode */
	unsigned int inode_rel_block_num = local_inode_index * sizeof(inode) / ext2->bytes_per_block;
	/* the byte offset of the inode in the block */
	unsigned int inode_rel_block_byte_offset = local_inode_index * sizeof(inode) % ext2->bytes_per_block;

	/* read the block from the inode table that contains the inode */
	char block[ext2->bytes_per_block];
	block_read(ext2, block, bg_d.bg_inode_table + inode_rel_block_num);

	/* read (and return) the inode from the block */
	*in = *(inode*)(block + inode_rel_block_byte_offset);
}
