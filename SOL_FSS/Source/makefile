CC=gcc
CFLAGS=-g

all: client server

rall: client server socket

client: client.c client_api.c
	$(CC) $(CFLAGS)  client.c client_api.c client_globals.c -o client

server: server.c server_globals.c  hash_table.c comm.c  conc_elem.c conc_queue.c cache.c server_util.c
	$(CC) $(CFLAGS) -pthread server.c cache.c server_util.c server_globals.c  hash_table.c comm.c  conc_elem.c conc_queue.c -o server

socket: mysocket
	rm mysocket

clean: 
	rm client
	rm server
	