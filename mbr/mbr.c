#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include "fs_names.h"

void parse_mbr(FILE* f) {
	char *buffer = (char*)malloc(16*4+2);
	fseek(f, 446, SEEK_SET);
	
	fread(buffer, 1, 16*4+2, f);
	unsigned short signature = *((unsigned short*)&buffer[16*4]);
	if (signature != 0xaa55) {
		printf("Note: mbr signature: %#x (bad)\n", signature);
	}

	int i, found = 0;
	for (i = 0; i < 4; ++i) {
		char* current_record = buffer + i * 16;

		unsigned int bytes_per_sector = 512; //hard coded
		unsigned int lba = *((int*)(current_record+8));
		unsigned int sectors = *((int*)(current_record+12));
		unsigned char type = (unsigned char)current_record[4];

		if (type != 0x00) {
			printf("%d: ", i);
			printf("Boot: %#x | ", (unsigned char)current_record[0]);
			printf("Type: %#x | ", type);

			printf("LBA: %#x | ", lba);
			printf("Sectors: %#x\n", sectors);

			//unsigned int partition_addr = lba * bytes_per_sector;

			found++;
		}
	}
	
	if (!found) {
		printf("No partitions found.\n");
	}
		
	free(buffer);	
}

int main(int argc, char *argv[]) {	
	FILE* f = fopen(argv[1], "r");

	parse_mbr(f);

	fclose(f);
	
	return 0;
}