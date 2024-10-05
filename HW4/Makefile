CFLAGS = -c -Wall
CC = gcc

all: hw4

hw4: hw4.o
	${CC} hw4.o -o hw4

hw4.o: hw4.c
	${CC} ${CFLAGS} hw4.c

clean:
	rm -f *.o *~ hw4
