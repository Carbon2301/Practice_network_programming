CFLAGS = -c -Wall
CC = gcc
LIBS = -lm

all: client server

debug: CFLAGS += -g
debug: client server

client: client1.o
	${CC} client1.o -o client

server: server1.o
	${CC} server1.o -o server

client1.o: client1.c
	${CC} ${CFLAGS} client1.c

server1.o: server1.c
	${CC} ${CFLAGS} server1.c

clean:
	rm -f *.o *~ client server
# Cu phap chay server : ./server 4000
#Cu phap chay client : ./client 127.0.0.1 4000 
# xoa cac file thuc thi chay lai tu dau dung lenh: make clean sau do chay lenh:make
# Chuong trinh thiet ke cho 1 server va 1 client 
