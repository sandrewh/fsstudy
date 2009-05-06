CFLAGS=-g -Wall

gpt : gpt.c gpt.h crc32.o
	gcc $(CFLAGS) -o gpt gpt.c crc32.o

clean :
	rm *.o
	rm gpt
	rm -r gpt.dSYM/

crc32.o : crc32.c