#pragma once

#define MAXSOCKETNAME 100
#include<sys/socket.h>
#include "../Headers/cache.h"

#define SOCKETPATHMAX 100 //lunghezza del path massimo per raggiungere la socket


/*Flags & Globals*/
extern char socketname[MAXSOCKETNAME];
extern int num_workers;
extern int memory_dimension;
extern int num_max_file;//numero massimo di file memorizzabili
extern int wait_time;//tempo di attesa tra richieste successive,associato a -t
extern int fd_socket;//file descriptor della socket
extern int fd_conn; //file descriptor della connessione
extern char * separator; 
extern struct sockaddr_un sa; //socket
extern int p_flag;
extern FILE * filelog;
extern char * logstring;
extern int sighintquit;
extern int sighup;
extern pthread_mutex_t mutexqueue;
extern pthread_cond_t condvqueue;
extern cache * mycache;