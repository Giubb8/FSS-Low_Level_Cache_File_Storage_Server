#pragma once
#include<pthread.h>
#include"../server_globals.h"
#include"./hash_table.h"
#include"./conc_queue.h"
#include"../server_util.h"
#include"./conc_list.h"
#include"../comm.h"


/*################## STRUCTS ###################*/


/* lock con variabile condizione associata */
typedef struct mylock{
    pthread_mutex_t mutex; //lock
    pthread_cond_t condition; //variabile condizione
}mylock;

/* Struct per memorizzare info dei thread */
typedef struct threadstat{
    int thread_id;
    int num_op;
}threadstat;

typedef struct filestats{
    int num_read;
    int num_write;
    float avg_size_read;
    float avg_size_write;
    int num_lock;
    int num_unlock;
    int num_openlock;
    int num_close;
    int dim_max;
    int num_max_file;
    int num_replaced;
    int num_query;
    int max_client;
}filestats;



/*struttura del file */
typedef struct myfile{
    char* filename;             //nome
    int size;                   //dimensione del file
    void* content;              //contenuto
    int wholocked;              //file descriptor di chi detiene la lock <0 se il file senza lock
    llist * who_opened;    //lista dei client che hanno aperto il file 
    mylock mutex;               //struttura lock
}myfile;


/*cache */
typedef struct cache{
    icl_hash_t * files_hash_table;  //tabella hash associata alla cache 
    int max_conn;                   //numero massimo di connessioni che puo gestire la cache 
    int max_mem;                    //massima memoria occupabile
    int max_files;                  //numero massimo di file
    int occupied_memory;            //memoria attualmente occupata in cache
    int num_files;                  //numero di file attualmente nella cache 
    pthread_mutex_t cache_mutex;   //mutex legata alla cache TODO forse * ?
    conc_queue * filenamequeue;     //lista dei nomi dei file nella cache,per farla fifo
}cache;

/*#####################  FUNZIONI  ####################*/
cache * create_cache();
int initmylock(mylock * lock);
int OpenFile( cache * cache,char * filename,int clientfd,int o_create,int o_lock);
int AppendTo( cache * cache,char * filename,int clientfd,char * content);
int CloseFile(cache * cache,char * filename,int sizefilename,int clientfd);
int ReadFile(cache *cache,char * filename,int clientfd);
int UnlockFile(cache * cache,char * filename,int clientfd);
int LockFile(cache * cache,char * filename,int clientfd);
int ReadNFile(cache *cache,int n,int clientfd);
int RemoveFile(cache * cache,char * filename,int clientfd);
int destroy_cache(cache * cache);

