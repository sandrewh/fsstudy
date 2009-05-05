#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gpt.h"
#include "crc32.h"

// TODO: use packed structs for partitions entry
// TODO: Current LBA, Backup LBA, First Usable LBA, Last Usable LBA 

void print_name_from_pretty_guid(char *pretty_guid)
{
	if (!strcmp(pretty_guid, "024DEE41-33E7-11D3-9D69-0008C781F39F"))
		printf("MBR");
	else if (!strcmp(pretty_guid, "C12A7328-F81F-11D2-BA4B-00A0C93EC93B"))
		printf("EFI System");
	else if (!strcmp(pretty_guid, "21686148-6449-6E6F-744E-656564454649"))
		printf("BIOS Boot");
	else if (!strcmp(pretty_guid, "E3C9E316-0B5C-4DB8-817D-F92DF00215AE"))
		printf("MS Reserved");
	else if (!strcmp(pretty_guid, "EBD0A0A2-B9E5-4433-87C0-68B6B72699C7"))
		printf("Microsoft/Linux Basic Data");
	else if (!strcmp(pretty_guid, "5808C8AA-7E8F-42E0-85D2-E1E90434CFB3"))
		printf("MS LDM Metadata");
	else if (!strcmp(pretty_guid, "AF9B60A0-1431-4F62-BC68-3311714A69AD"))
		printf("MS LDM Data");
	else if (!strcmp(pretty_guid, "A19D880F-05FC-4D3B-A006-743F0F84911E"))
		printf("Linux RAID");
	else if (!strcmp(pretty_guid, "0657FD6D-A4AB-43C4-84E5-0933C84B4F4F"))
		printf("Linux Swap");
	else if (!strcmp(pretty_guid, "E6D6D379-F507-44C2-A23C-238F2A3DF928"))
		printf("Linux LVM");
	else if (!strcmp(pretty_guid, "8DA63339-0007-60C0-C436-083AC8230908"))
		printf("Linux Reserved");
	else if (!strcmp(pretty_guid, "48465300-0000-11AA-AA11-00306543ECAC"))
		printf("Apple HFS+");
	else if (!strcmp(pretty_guid, "55465300-0000-11AA-AA11-00306543ECAC"))
		printf("Apple UFS");
	else if (!strcmp(pretty_guid, "6A898CC3-1DD2-11B2-99A6-080020736631"))
		printf("Apple ZFS");
	else if (!strcmp(pretty_guid, "52414944-0000-11AA-AA11-00306543ECAC"))
		printf("Apple RAID (online)");
	else if (!strcmp(pretty_guid, "52414944-5F4F-11AA-AA11-00306543ECAC"))
		printf("Apple RAID (offline)");
	else if (!strcmp(pretty_guid, "426F6F74-0000-11AA-AA11-00306543ECAC"))
		printf("Apple Boot");
	else if (!strcmp(pretty_guid, "4C616265-6C00-11AA-AA11-00306543ECAC"))
		printf("Apple Label");
	else if (!strcmp(pretty_guid, "5265636F-7665-11AA-AA11-00306543ECAC"))
		printf("Apple TV Recovery");
	else printf("Unknown");
}

/* print pretty 128-bit guid */
void sprint_guid(char *dst, unsigned char* b)
{
	sprintf(dst, "%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
		b[3], b[2], b[1], b[0], /* first block is reversed */
		b[5], b[4],	/* also reversed */
		b[7], b[6],	/* reversed */
		b[8], b[9],	/* NOT reversed */
		b[10], b[11], b[12], b[13], b[14], b[15]); /* NOT reversed */
}

/* print size in a human readable format */
void print_human_size(float size)
{
	if (!size) 
	{
		printf("%d", 0);
		return;
	}
	
	char a = 'T';
	if (size < 1024.0) a = 'B'; 
	else if ((size/=1024.0) < 1024.0) a = 'K';
	else if ((size/=1024.0) < 1024.0) a = 'M';
	else if ((size/=1024.0) < 1024.0) a = 'G';
	else size/=1024.0;

	printf("%.1f %c", size, a);
}

/* parse the gpt and display info */
void parse_gpt(FILE* f) {
	GPT_HEADER header;
	
	/* read LBA 1 from disk */
	fseek(f, 1*BYTES_PER_LBA, SEEK_SET);
	//fread(buf, 1, BUF_SIZE, f);
	fread(&header, 1, sizeof(header), f);

	/* check for EFI signature */
	if (header.efi_signature != EFI_SIG)
	{
		printf("Invalid EFI Signature.\n");
		return;
	}

	/* check for rev. other than 0 0 1 0 */
	if (header.revision != EFI_REV)
	{
		printf("Warning: GPT Rev. %08x found.  Only tested Rev. %08x.\n", header.revision, EFI_REV);		
	}

	/* check header size */
	if (header.size != 92)
	{
		printf("Warning: Header Size = %u (expected 92)\n", header.size);
	}

	/* check header crc32 */
	uint32_t recorded_header_crc32 = header.crc32;
	
	header.crc32 = 0;
	uint32_t calc_header_crc32 = crc32(0, (unsigned char*)&header, header.size);
	if (calc_header_crc32 != recorded_header_crc32)
	{
		printf("Header Checksum: BAD (read: %08x / calc: %08x)\n", recorded_header_crc32, calc_header_crc32);
				
//	} else {
//		printf("Header Checksum: Good\n");
	}
	header.crc32 = recorded_header_crc32; /* restore crc32, after calculation */
		
	/* print disk guid */
	char pretty_guid[PRETTY_GUID_LEN+1];
	sprint_guid(pretty_guid, header.disk_uuid);
	printf("Disk UUID: %s\n\n", pretty_guid);
	
	if (header.partition_entries_first_lba != 2)
	{
		printf("Warning: Table Start LBA = %llu (should be 2)\n", header.partition_entries_first_lba);
	}
	
//	printf("Partition Slots: %u\n", header.num_partition_entries);

//	printf("Slot Size: %u\n", header.partition_entry_size);
	if (header.partition_entry_size != 128)
	{
		printf("Interesting: Slot Size = %u (should be 128, but Apple said it might change)\n", header.partition_entry_size);
	}

	if (header.partition_entry_size > BUF_SIZE)
	{
		printf("Error: sizeof_partition_entries (%u) > BUF_SIZE (%u), should be 128.\n", header.partition_entry_size, BUF_SIZE);
		return;
	}

	/* LBA 2 */
	fseek(f, 2*BYTES_PER_LBA, SEEK_SET); 
	
	uint32_t calc_table_crc32 = 0;
	
	unsigned char buf[BUF_SIZE];
	int i;
	for (i=0; i<(header.num_partition_entries); i++)
	{		
		fread(buf, 1, header.partition_entry_size, f);

		calc_table_crc32 = crc32(calc_table_crc32, buf, header.partition_entry_size);

		int j;	
		/* a type guid of all 0's is unused (in this case is interpreted to mean "end of entries") */
		for (j=0;j<16;j++) if (buf[j]) break; /* break early if non-zero byte found */
		if (j>=16) continue; /* type guid was all 0's, continue */

		printf("Partition %d:\n", i);
		
		printf("Name:\t\t\"");
		/* hack to read utf-16 as ascii */
		for (j=56;j<56+72;j+=2)
		{
			if (!buf[j]) break;
			printf("%c", buf[j]);
		}
		printf("\"\n");

		sprint_guid(pretty_guid, buf+0);
		printf("Type GUID:\t%s (", pretty_guid);
		print_name_from_pretty_guid(pretty_guid);
		printf(")\n");

		sprint_guid(pretty_guid, buf+16);
		printf("Unique GUID:\t%s\n", pretty_guid);
		
		long long unsigned int first_lba, last_lba;
		first_lba = *(long long unsigned int*)(buf+32);
		last_lba = *(long long unsigned int*)(buf+40);
		long long unsigned int size_b = (last_lba-first_lba+1)*0x200;

		printf("LBA Range:\t[%#llx, %#llx]\n", first_lba, last_lba);
		printf("Size:\t\t");
		print_human_size(size_b);
		if (size_b >= 1024) printf(" (%llu)", size_b);
		printf("\n");

		printf("Attributes:\t");
		for (j=48;j<48+8;j++)
		{
			printf("%02x ", buf[j]);
		}
		printf("\n\n");
	}
	
	if (calc_table_crc32 != header.partition_table_crc32)
	{
		printf("Table Checksum: BAD (read: %08x / calc: %08x)\n", header.partition_table_crc32, calc_table_crc32);
				
//	} else {
//		printf("Table Checksum: Good\n");
	}
	
}

int main(int argc, char *argv[]) {	
	FILE* f = fopen(argv[1], "r");
	if (!f)
	{
		printf("fopen: failed opening %s\n", argv[1]);
		return 1;
	}
	parse_gpt(f);
	fclose(f);
	
	return 0;
}