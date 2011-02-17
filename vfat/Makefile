CFLAGS=-g -Wall -m32

vfat : vfat.o main.o cluster.o fentry.o sector.o ../util/cache.o ../util/hash.o
	gcc $(CFLAGS) -o vfat main.o vfat.o cluster.o fentry.o sector.o ../util/cache.o ../util/hash.o

clean :
	rm *.o
	rm vfat
	rm -r vfat.dSYM/	

vfat.h : fentry.o part_info.h cluster.o
vfat.o : vfat.c vfat.h

main.o : main.c vfat.h

cluster.h : part_info.h
cluster.o : cluster.c cluster.h

fentry.o : fentry.c fentry.h

sector.o : sector.h
