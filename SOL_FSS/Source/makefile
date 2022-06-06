CC=gcc
CFLAGS=-g

all: client server

rall: client server socket

client: client.c
	$(CC) $(CFLAGS) -o client client.c

server: server.c
	$(CC) $(CFLAGS) -o server -pthread server.c 

socket: mysocket
	rm mysocket

clean: 
	rm client
	rm server
	