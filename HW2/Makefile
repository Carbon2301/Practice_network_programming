CFLAGS = -c -Wall
CC = gcc

all: lookup

lookup: lookup.o
	${CC} lookup.o -o lookup

lookup.o: lookup.c
	${CC} ${CFLAGS} lookup.c

clean:
	rm -f *.o *~ lookup
