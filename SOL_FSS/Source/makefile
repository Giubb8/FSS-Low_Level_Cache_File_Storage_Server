CC=gcc
CFLAGS=-g
PTHREAD=-pthread

CLIENT_DEP=client.c client_api.c client_globals.c 
SERVER_DEP=server.c cache.c server_util.c conc_list.c server_globals.c  hash_table.c comm.c  conc_elem.c conc_queue.c

all: client server 

rall: client server socket

client: $(CLIENT_DEP)
	$(CC) $(CFLAGS)  $(CLIENT_DEP) -o $@

server: $(SERVER_DEP)
	$(CC) $(CFLAGS) $(PTHREAD) $(SERVER_DEP)  -o $@

socket: mysocket
	rm mysocket

clean: 
	rm client
	rm server
	
cleanall:
	rm client
	rm server
	rm -r dir
	rm -r dir_replace
	rm -r dir_replace_2
	rm mysocket

test1: 
	make all
	chmod +x ../Script/script_1.sh
	bash ../Script/script_1.sh
	bash ../Script/statistiche.sh

test2: 
	make all
	chmod +x ../Script/script_2.sh
	bash ../Script/script_2.sh
	bash ../Script/statistiche.sh

test3: 
	make all
	chmod +x ../Script/script_3.sh
	bash ../Script/script_3.sh
	bash ../Script/statistiche.sh
	make socket
