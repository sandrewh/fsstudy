#include <stdlib.h>
#include <math.h>

#include "ntfs.h"

//http://www.linux-ntfs.org/doku.php?id=howto:hexedityourway
//http://technet.microsoft.com/en-us/library/cc781134.aspx#w2k3tr_ntfs_how_dhao

ntfs_info*
ntfs_mount (FILE* f, unsigned int partition_start_sector)
{
	ntfs_info *ntfs = malloc(sizeof(ntfs_info));
	ntfs->bpb.bytes_per_sector = 512;
	ntfs->volume_offset = partition_start_sector * ntfs->bpb.bytes_per_sector;
	ntfs->f = f;
	
	if (fseek(ntfs->f, ntfs->volume_offset, SEEK_SET))
	{
		printf("ntfs_mount: fseek: failed seeking to volume_offset\n");
		free(ntfs); ntfs = 0; return 0;
	}
	fread(&ntfs->bpb, sizeof(ntfs->bpb), 1, ntfs->f);
	
	if (ntfs->bpb.reserved_sectors)
	{
		printf("ntfs_mount: reserved_sectors = %d (should be 0)\n", ntfs->bpb.reserved_sectors);
		free(ntfs); ntfs = 0; return 0;
	}
	
	if (ntfs->bpb.zero[0] | ntfs->bpb.zero[1] | ntfs->bpb.zero[2] | ntfs->bpb.zero[3] | ntfs->bpb.zero[4]
		| ntfs->bpb.zero_1[0] | ntfs->bpb.zero_1[1]
		| ntfs->bpb.zero_2[0] | ntfs->bpb.zero_2[1] | ntfs->bpb.zero_2[2] | ntfs->bpb.zero_2[3])
	{
		printf("ntfs_mount: zero'd data was not zero'd\n");
		free(ntfs); ntfs = 0; return 0;
	}		
	
	ntfs->bytes_per_cluster = ntfs->bpb.bytes_per_sector * ntfs->bpb.sectors_per_cluster;
	
	if (ntfs->bpb._clusters_per_mft_record < 0)
		ntfs->clusters_per_mft_record = pow(2, abs(ntfs->bpb._clusters_per_mft_record));
	else ntfs->clusters_per_mft_record = ntfs->bpb._clusters_per_mft_record;

	if (ntfs->bpb._clusters_per_index_buffer < 0)
		ntfs->clusters_per_index_buffer = pow(2, abs(ntfs->bpb._clusters_per_index_buffer));
	else ntfs->clusters_per_index_buffer = ntfs->bpb._clusters_per_index_buffer;

	return ntfs;
}

void
ntfs_umount (ntfs_info *ntfs)
{
	free(ntfs); ntfs = 0;
}