#ifndef VFAT_H
#define VFAT_H

#include <stdio.h>

#include "fentry.h"
#include "part_info.h"
#include "cluster.h"

#define USE_CACHE 0
#define CREATE_FILE_ON_WRITE 1

void print_human_size(float size);

f_entry* fentry_from_path(part_info *p, char *path);

f_entry* enumerate_dir(part_info* p, unsigned int dir_cluster, int callback(f_entry f, void *arg), void *arg);
//int search_dir_callback(f_entry file, void *arg);
f_entry* search_dir(part_info *p, unsigned int dir_cluster, char* search_name);

void file_size_to_disk(part_info *p, f_entry *f);

int read_file(part_info *p, f_entry *f, char* buffer, size_t size, off_t offset);
//int read_dir_callback(f_entry file, void *arg);
unsigned int read_dir(part_info *p, char* path, f_entry *entries, size_t size, off_t offset);

int dir_rem_entry(part_info *p, f_entry *f);
int dir_add_entry(part_info *p, f_entry *d, f_entry* f);
int file_rem(part_info *p, char *path);
int entry_create(part_info *p, char *path, unsigned char attr);

void count_unused_cluster_not_zerod(part_info *p);
void zero_unused_clusters(part_info *p);

void first_cluster_to_disk(part_info *p, f_entry *f);
int write_file(part_info *p, f_entry *f, char* buffer, size_t size, off_t offset);

part_info vfat_mount(FILE* f, unsigned int partition_start_sector);
void vfat_umount(part_info *p);

#endif