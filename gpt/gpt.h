#ifndef GPT
#define GPT_H value

#include <stdint.h>
#include <stddef.h>

const uint64_t EFI_SIG = 0x5452415020494645ULL; //"EFI PART"
const uint32_t EFI_REV = 0x10000;
const unsigned int BUF_SIZE = 512;
const unsigned int PRETTY_GUID_LEN =	16	/* hex digits */
										+ 4	/* dashes */;
const unsigned int BYTES_PER_LBA = 0x200;

typedef struct
{
	uint64_t efi_signature;
	uint32_t revision;
	uint32_t size;
	uint32_t crc32;
	uint32_t zero;
	uint64_t current_lba;
	uint64_t backup_lba;
	uint64_t first_usable_lba;
	uint64_t last_usable_lba;	
	uint8_t disk_uuid[16];
	uint64_t partition_entries_first_lba;
	uint32_t num_partition_entries;
	uint32_t partition_entry_size;
	uint32_t partition_table_crc32;
	uint8_t zeros[420];
} __attribute__((__packed__)) GPT_HEADER;

#endif
