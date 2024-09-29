CFLAGS = -c -Wall
CC = gcc

all: hw3

hw3: hw3.o
	${CC} hw3.o -o hw3

hw3.o: hw3.c
	${CC} ${CFLAGS} hw3.c

clean:
	rm -f *.o *~ hw3
