#ifndef NTFS_INFO_H
#define NTFS_INFO_H

typedef struct
{
	unsigned char jmp[3];
	unsigned char oem[8];
	unsigned short bytes_per_sector;
	unsigned char sectors_per_cluster;
	unsigned short reserved_sectors;
	unsigned char zero[5];
	unsigned char media;
	unsigned char zero_1[2];
	unsigned char unused[8];
	unsigned char zero_2[4];
	unsigned char unused_1[4];
	unsigned long long int total_sectors;
	unsigned long long int mft_lcn;
	unsigned long long int mftMirr_lcn;
	char _clusters_per_mft_record;
	unsigned char unused_2[3];
	char _clusters_per_index_buffer;
	unsigned char unused_3[3];
	unsigned long long int volume_serial;
} __attribute__((__packed__)) bpb_info;

typedef struct
{
	FILE *f;
	unsigned int volume_offset;
	unsigned int bytes_per_cluster;
	unsigned int clusters_per_mft_record;	
	unsigned int clusters_per_index_buffer;	
	bpb_info bpb;
} ntfs_info;

#endif

