#include<stdio.h>
#include "../Headers/cache.h"
#include "../Headers/server_globals.h"

char socketname[MAXSOCKETNAME]; //nome della socket 
int num_workers=0; //numero di thread worker
int memory_dimension=0; //dimensione della memoria associata alla cache
int num_max_file=0;//numero massimo di file memorizzabili
int wait_time=0;//tempo di attesa tra richieste successive,associato a -t
int fd_socket=-1;//file descriptor della socket
int fd_conn=-1;//file descriptor della connessione 
char * separator="~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";
int p_flag=1; //flag per abilitare le stampe
FILE * filelog; //file descriptor del file di log
char * logstring; //stringa da passare al file di log
int sighintquit=0; //flag per segnalare il segnale sigint o sigquit
int sighup=0; //flag per segnalare il segnale sighup
pthread_mutex_t mutexqueue; //mutexlock sulla coda di fd dei clients 
pthread_cond_t condvqueue; //condition variables sulla coda di fd dei clients
cache * mycache=NULL; //cache del server