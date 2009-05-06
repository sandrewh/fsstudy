#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const int MBR_SIG_SIZE = 2;
const int BYTES_PER_RECORD = 16;
const int MAX_NUM_RECORDS = 4;

void parse_part(FILE *f, unsigned char *current_record, unsigned int lba_rel);

void
print_part_type (unsigned char type)
{
	switch (type)
	{
		case 01: printf("FAT12"); break;
		case 04: printf("FAT16<32M"); break;		
		case 05: printf("Extended"); break;		
		case 06: printf("FAT16"); break;		
		case 07: printf("NTFS"); break;		
		case 0x0B: printf("WIN95 FAT32"); break;		
		case 0x0C: printf("WIN95 FAT32 (LBA)"); break;		
		case 0x0E: printf("WIN95 FAT16 (LBA)"); break;		
		case 0x0F: printf("WIN95 Ext'd (LBA)"); break;		
		case 0x11: printf("Hidden FAT12"); break;		
		case 0x14: printf("Hidden FAT16<32M"); break;		
		case 0x16: printf("Hidden FAT16"); break;		
		case 0x17: printf("Hidden NTFS"); break;		
		case 0x1B: printf("Hidden WIN95 FAT32"); break;		
		case 0x1C: printf("Hidden WIN95 FAT32 (LBA)"); break;		
		case 0x1E: printf("Hidden WIN95 FAT16 (LBA)"); break;		
		case 0x82: printf("Linux Swap"); break;		
		case 0x83: printf("Linux"); break;		
		case 0x85: printf("Linux Ext'd"); break;		
		case 0x86: case 0x87: printf("NTFS Vol. Set"); break;		
		case 0x8E: printf("Linux LVM"); break;		
		case 0x9f: printf("BSD/OS"); break;		
		case 0xa5: printf("FreeBSD"); break;		
		case 0xa6: printf("OpenBSD"); break;		
		case 0xa9: printf("NetBSD"); break;		
		case 0xeb: printf("BeOS fs"); break;		
		case 0xee: printf("EFI GPT"); break;		
		case 0xef: printf("EFI FAT?"); break;
		default: printf("?");
	}
}

void
print_human_size (float size)
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

char
get_boot_char (unsigned char b)
{
	switch (b)
	{
		case 0x00: return 'N';
		case 0x80: return 'Y';
		default: return '?';
	}
}

/* see: http://en.wikipedia.org/wiki/Extended_boot_record */
void
parse_ebr (FILE *f, unsigned int lba)
{
	const int BUFFER_SIZE = (BYTES_PER_RECORD * MAX_NUM_RECORDS) + MBR_SIG_SIZE;	

	unsigned char buffer[BUFFER_SIZE];
	fseek(f, lba*0x200+0x1be, SEEK_SET);
	fread(buffer, 1, BUFFER_SIZE, f);
	
	unsigned char *current_record = buffer;

	/* print info about the first part */
	parse_part(f, current_record, lba);
	
	current_record += 16;
	int j;
	for (j=0;j<16;j++) if (current_record[j]) break; /* break early if non-zero byte found */
	if (j>=16) return; /* 2nd part was all 0's, return */
	
	unsigned int next_lba_rel = *(int*)(current_record+8);	
	
	/* TODO: traverse the ebr chain */
	parse_ebr(f, lba+next_lba_rel);	
}

void
parse_part (FILE *f, unsigned char *current_record, unsigned int lba_rel)
{
	unsigned char boot = (unsigned char)current_record[0];
	unsigned char type = (unsigned char)current_record[4];
	unsigned int lba = *(int*)(current_record+8);
	unsigned int sectors = *(int*)(current_record+12);

	if (type != 0x00) {
		printf("Boot:\t\t%c (%#x)\n", get_boot_char(boot), boot);
		printf("Type:\t\t");
		print_part_type(type);
		printf(" (%#x)\n", type);

		if (lba_rel)
		{
			printf("LBA:\t\t%#x (+%#x)\n", lba+lba_rel, lba);
		} else {
			printf("LBA:\t\t%#x\n", lba);			
		}
		printf("Sectors:\t%#x (", sectors);
		print_human_size((float)((unsigned long long int)sectors*0x200));
		printf(")\n\n");
		
		/* handle extended partitions */
		if (type == 0x05)
		{
			printf("Warning: Extended Partition Detected - this feature not tested\n");
			parse_ebr(f, lba);
		}
	}
}

void
parse_mbr (FILE* f) {
	const int MBR_OFFSET = 0x1b8;
	const int DISC_SIG_SIZE = 4;
	const int NULLS_SIZE = 2;
	const int BUFFER_SIZE = DISC_SIG_SIZE+NULLS_SIZE+(BYTES_PER_RECORD*MAX_NUM_RECORDS)+MBR_SIG_SIZE;

	unsigned char buffer[BUFFER_SIZE];
	fseek(f, MBR_OFFSET, SEEK_SET);
	fread(buffer, 1, BUFFER_SIZE, f);
	
	unsigned short signature = *(unsigned short*)(buffer+BUFFER_SIZE-MBR_SIG_SIZE);
	if (signature != 0xaa55)
	{
		printf("Note: mbr signature: %#x (bad)\n", signature);
	}
	
	printf("Disk Signature: ");
	int i;
	for (i=0;i<DISC_SIG_SIZE;i++)
	{
		printf("%02x", buffer[i]);
	}
	printf("\n");
	
	for (i = 0; i < MAX_NUM_RECORDS; ++i)
	{
		unsigned char *current_record = buffer + DISC_SIG_SIZE + NULLS_SIZE + (BYTES_PER_RECORD * i);

		parse_part(f, current_record, 0);
	}
}

int
main (int argc, char *argv[]) {	
	FILE* f = fopen(argv[1], "r");
	if (!f)
	{
		printf("fopen: failed opening %s\n", argv[1]);
		return 1;
	}

	parse_mbr(f);
	fclose(f);

	return 0;
}