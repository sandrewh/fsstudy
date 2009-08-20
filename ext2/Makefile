CFLAGS=-g -Wall

ext2 : main.o ext2.o block.o ext2_info.h inode.h ../util/cache.o ../util/hash.o
	gcc $(CFLAGS) -o ext2 main.o ext2.o block.o inode.c ../util/cache.o ../util/hash.o

clean :
	rm *.o
	rm ext2
	rm -r ext2.dSYM/

main.o : main.c
ext2.o : ext2.c ext2.h
block.o : block.c block.h
inode.o : inode.c inode.h

cache.o : cache.c cache.h
	make -C ../util/
	
hash.o : ../util/hash.c ../util/hash.h
	make -C ../util/