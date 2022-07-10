#include<stdio.h>
#include "../Headers/server_globals.h"
#include "../Headers/DataStructures/cache.h"

/*################################ VARIABILI GLOBALI DEL SERVER  ############################################*/

char socketname[MAXSOCKETNAME]; //nome della socket 
int num_workers=0; //numero di thread worker
int memory_dimension=0; //dimensione della memoria associata alla cache
int num_max_file=0;//numero massimo di file memorizzabili
int wait_time=200;//tempo di attesa tra richieste successive,associato a -t
int fd_socket=-1;//file descriptor della socket
int fd_conn=-1;//file descriptor della connessione 
char * separator="~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";
char * opseparator="-------------------------------------------------------------------------------------------\n";
int p_flag=1; //flag per abilitare le stampe
FILE * filelog; //file descriptor del file di log
char * logstring; //stringa da passare al file di log

int sighintquit=0; //flag per segnalare il segnale sigint o sigquit
int sighup=0; //flag per segnalare il segnale sighup
pthread_mutex_t mutex_clients_queue; //mutexlock sulla coda di fd dei clients 
pthread_cond_t condv_clients_queue; //condition variables sulla coda di fd dei clients
cache * mycache=NULL; //cache del server
conc_queue* log_queue;// coda per i messaggi da loggare
int comm_flag=0;//flag per abilitare le stampe delle lock e signal
tolog_struct tolog;
int p_client=0;//int per memorizzare il numero massimo di client connessi contemporaneamente
pthread_mutex_t p_client_mutex=PTHREAD_MUTEX_INITIALIZER;//mutex associata
pthread_mutex_t tolog_struct_mutex=PTHREAD_MUTEX_INITIALIZER;//mutex associata alla struttura da loggare