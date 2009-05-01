#ifndef EXT2_INFO_H
#define EXT2_INFO_H

#include <stdio.h>

#define EXT2_ROOT_INO 2
#define EXT2_FT_DIR 2

#define EXT2_SUPER_BLOCK_MAGIC	0xef53

//-- s_rev_level
#define EXT2_GOOD_OLD_REV	0
#define EXT2_DYNAMIC_REV	1

//-- file format --
#define EXT2_S_IFSOCK	0xC000	//socket
#define EXT2_S_IFLNK	0xA000	//symbolic link
#define EXT2_S_IFREG	0x8000	//regular file
#define EXT2_S_IFBLK	0x6000	//block device
#define EXT2_S_IFDIR	0x4000	//directory
#define EXT2_S_IFCHR	0x2000	//character device
#define EXT2_S_IFIFO	0x1000	//fifo
//-- process execution user/group override --
#define EXT2_S_ISUID	0x0800	//Set process User ID
#define EXT2_S_ISGID	0x0400	//Set process Group ID
#define EXT2_S_ISVTX	0x0200	//sticky bit
//-- access rights --
#define EXT2_S_IRUSR	0x0100	//user read
#define EXT2_S_IWUSR	0x0080	//user write
#define EXT2_S_IXUSR	0x0040	//user execute
#define EXT2_S_IRGRP	0x0020	//group read
#define EXT2_S_IWGRP	0x0010	//group write
#define EXT2_S_IXGRP	0x0008	//group execute
#define EXT2_S_IROTH	0x0004	//others read
#define EXT2_S_IWOTH	0x0002	//others write
#define EXT2_S_IXOTH	0x0001	//others execute

typedef struct
{
	unsigned int s_inodes_count;
	unsigned int s_blocks_count;
	unsigned int s_r_blocks_count;
	unsigned int s_free_blocks_count;
	unsigned int s_free_inodes_count;
	unsigned int s_first_data_block;
	unsigned int s_log_block_size;
	unsigned int s_log_frag_size;
	unsigned int s_blocks_per_group;
	unsigned int s_frags_per_group;
	unsigned int s_inodes_per_group;
	unsigned int s_mtime;
	unsigned int s_wtime;
	
	unsigned short s_mnt_count;
	unsigned short s_max_mnt_count;
	unsigned short s_magic;
	unsigned short s_state;
	unsigned short s_errors;
	unsigned short s_minor_rev_level;
	
	unsigned int s_lastcheck;
	unsigned int s_checkinterval;
	unsigned int s_creator_os;
	unsigned int s_rev_level;
	
	unsigned short s_def_resuid;
	unsigned short s_def_resgid;
	
	//-- EXT2_DYNAMIC_REV Specific --
	
	unsigned int s_first_ino;
	unsigned short s_inode_size;
	unsigned short s_block_group_nr;
	unsigned int s_feature_compat;
	unsigned int s_feature_incompat;
	unsigned int s_feature_ro_compat;
	unsigned char s_uuid[16];
	char s_volume_name[16];
	char s_last_mounted[64];
	unsigned int s_algo_bitmap;
	
} __attribute__((__packed__)) super_block;

typedef struct
{
	unsigned int bg_block_bitmap;
	unsigned int bg_inode_bitmap;
	unsigned int bg_inode_table;
	
	unsigned short bg_free_blocks_count;
	unsigned short bg_free_inodes_count;
	unsigned short bg_used_dirs_count;
	unsigned short bg_pad;
	
	unsigned char bg_reserved[12];
} __attribute__((__packed__)) bg_desc;

typedef struct
{
	unsigned short i_mode;
	unsigned short i_uid;
	
	unsigned int i_size;
	unsigned int i_atime;
	unsigned int i_ctime;
	unsigned int i_mtime;
	unsigned int i_dtime;
	
	unsigned short i_gid;
	unsigned short i_links_count;
	
	unsigned int i_blocks;
	unsigned int i_flags;
	unsigned int i_osd1;
	
	unsigned int i_block[15];
	
	unsigned int i_generation;
	unsigned int i_file_acl;
	unsigned int i_dir_acl;
	unsigned int i_faddr;

	unsigned char i_osd2[12];
} __attribute__((__packed__)) inode;

typedef struct
{
	unsigned int inode;
	unsigned short rec_len;
/*	16bit unsigned displacement to the next directory entry from the start
	of the current directory entry. This field must have a value at least
	equal to the length of the current record.
	The directory entries must be aligned on 4 bytes boundaries and there
	cannot be any directory entry spanning multiple data blocks. If an
	entry cannot completely fit in one block, it must be pushed to the
	next data block and the rec_len of the previous entry properly
	adjusted.	
*/
	
	unsigned char name_len;
	unsigned char file_type;
	char *name;
} __attribute__((__packed__)) directory_entry;

typedef struct 
{
	unsigned int volume_offset;
	unsigned int super_block_offset;
	super_block s;
	unsigned int bytes_per_block;
	FILE* f;
} ext2_info;

#endif