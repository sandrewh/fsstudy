#ifndef FENTRY_H
#define FENTRY_H

static const int FILE_ENTRY_SIZE = 0x20;

typedef struct {
	unsigned char h, min, s;
	unsigned char mon, d;
	unsigned int y;
} DateTime;

typedef struct {
	unsigned int first_cluster;
	unsigned int size_bytes;
	char name[13];
	char lfn[128];
	
	unsigned char attr_r, attr_h, attr_s, attr_v, attr_d, attr_a;

	DateTime created, accessed, modified;
	
	unsigned int dir_first_cluster;
	unsigned int dir_slot_offset;
	unsigned int dir_num_slots;
} f_entry;

unsigned char lfn_checksum(const unsigned char *pFcbName);
void f_entry_set_attr(f_entry *file, unsigned char attr);
unsigned char f_entry_get_attr(f_entry *file);
void datetime_decode(DateTime* dt, unsigned char d1, unsigned char d2, unsigned char t1, unsigned char t2);
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
