all : mbr vfat

mbr : mbr.c
	gcc -g -Wall -o mbr mbr.c
	
vfat : vfat.c
	gcc -g -Wall -o vfat vfat.c