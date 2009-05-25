CFLAGS=-g -Wall

hfsp : main.o hfsp.o
	gcc $(CFLAGS) -o hfsp main.o hfsp.o

clean :
	rm *.o
	rm hfsp
	rm -r hfsp.dSYM/

main.o : main.c
hfsp.o : hfsp.c hfsp.h
