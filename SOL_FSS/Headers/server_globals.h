#pragma once
#include<sys/socket.h>
#include "./DataStructures/cache.h"

#define MAXSOCKETNAME 100
#define SOCKETPATHMAX 100 //lunghezza del path massimo per raggiungere la socket
#define MAXPATH 100 //lunghezza del path massimo per raggiungere la socket

/* Struttura per le info da loggare sul server */
typedef struct tolog_struct{
    int maxconnection_tolog;//numero massimo di connessioni contemporanee raggiunto dalla cache 
    int maxcapacity_tolog; //numero massimo di memoria occupata raggiunto dalla cache 
    int maxnumfile_tolog;//numero massimo di file raggiunto dalla cache 
    int replaceop_tolog; //numero di volte in cui è stato usato l'algoritmo di rimpiazzo 
    int clientserved_tolog; //numero di client serviti dai thread workers //TODO da cancellare perche lo prendo il dato dallo script
}tolog_struct;


/*Flags & Globals*/
extern char socketname[MAXSOCKETNAME];
extern int num_workers; 
extern int memory_dimension;
extern int num_max_file;//numero massimo di file memorizzabili
extern int wait_time;//tempo di attesa tra richieste successive,associato a -t
extern int fd_socket;//file descriptor della socket
extern int fd_conn; //file descriptor della connessione
extern char * separator; 
extern char * opseparator;
extern struct sockaddr_un sa; //socket
extern int p_flag;
extern FILE * filelog;
extern char * logstring;
extern int sighintquit;
extern int sighup;
extern pthread_mutex_t mutex_clients_queue;
extern pthread_cond_t condv_clients_queue;
extern cache * mycache;
extern conc_queue * log_queue;// coda per i messaggi da loggare
extern int comm_flag; //flag per abilitare le stampe di comm
extern int p_client;
extern pthread_mutex_t p_client_mutex;
extern pthread_mutex_t tolog_struct_mutex;//mutex associata alla struttura da loggare
extern tolog_struct tolog;

/*codice per ogni operazione*/
enum opcode_{
    TURNOFF =0,
    OPEN = 1,
    READ = 2,
    READN = 3,
    WRITE = 4,
    REMOVE = 5,
    CLOSE = 6,
    APPEND = 7,
    LOCK = 8,
    UNLOCK = 9,
    
};
/* codici per gli errori */
enum errors_{
    ERROR = -1,
    FATAL_ERROR = -2,
    SUCCESS =0
};


/* Struttura per inviare le risposte al client */
typedef struct rep{
  int done;
  int more;
  int err;
}rep;
