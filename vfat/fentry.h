#ifndef FENTRY_H
#define FENTRY_H

#include <stdint.h>

static const int FILE_ENTRY_SIZE = 0x20;

typedef struct
{
	uint8_t h, min, s;
	uint8_t mon, d;
	uint16_t y;
} DateTime;

typedef struct
{
	uint32_t first_cluster;
	uint32_t size_bytes;
	char name[13];
	char lfn[128];
	
	uint8_t attr_r, attr_h, attr_s, attr_v, attr_d, attr_a;

	DateTime created, accessed, modified;
	
	uint32_t dir_first_cluster;
	uint32_t dir_slot_offset;
	uint32_t dir_num_slots;
} f_entry;

uint8_t lfn_checksum(const unsigned char *pFcbName);
void f_entry_set_attr(f_entry *file, uint8_t attr);
uint8_t f_entry_get_attr(f_entry *file);
void datetime_decode(DateTime* dt, uint8_t d1, uint8_t d2, uint8_t t1, uint8_t t2);
void f_entry_set_datetimes(f_entry *file, char* entry);
void from_utf16(char* buffer, int len);
void to_utf16(char* buffer, int len, int cap);
void f_entry_set_name_from_8_3(f_entry *file);

void f_entry_update_lfn(f_entry *file, char* entry);
void f_entry_convert_name(f_entry *file);
void fentry_set_unique_name_from_lfn(f_entry *f);
void fentry_convert_name_to_8_3(f_entry *f);
//f_entry* fentry_from_subpath(part_info *p, f_entry *d, char* path);

#endif
