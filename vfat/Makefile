CFLAGS=-g -Wall

vfat : vfat.o main.o cluster.o fentry.o cache.o sector.o
	gcc $(CFLAGS) -o vfat main.o vfat.o cluster.o fentry.o cache.o sector.o

clean :
	rm *.o
	rm vfat
	rm -r vfat.dSYM/	
	
vfat.h : fentry.o part_info.h cluster.o
vfat.o : vfat.c vfat.h

main.o : main.c vfat.h

cluster.h : part_info.h
cluster.o : cluster.c cluster.h cache.o

fentry.o : fentry.c fentry.h

cache.h : part_info.h
cache.o : cache.c cache.h

sector.o : sector.h cache.o
