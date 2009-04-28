CFLAGS=-g -Wall

ntfs : main.o ntfs.o
	gcc $(CFLAGS) -o ntfs main.o ntfs.o

clean :
	rm *.o
	rm ntfs
	rm -r ntfs.dSYM/

main.o : main.c
ntfs.o : ntfs.c ntfs.h
