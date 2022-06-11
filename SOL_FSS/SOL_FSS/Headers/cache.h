#pragma once
#include<pthread.h>
#include"hash_table.h"
#include"conc_queue.h"







/*################## Structs ###################*/



/* lock con variabile condizione associata */
typedef struct mylock{
    pthread_mutex_t mutex; //lock
    pthread_cond_t condition; //variabile condizione
}mylock;

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
    char* filename; //nome
    int size;       //dimensione del file
    void* content;  //contenuto
    int wholocked;  //file descriptor di chi detiene la lock
    mylock mutex;   //struttura lock
}myfile;


/*cache */
typedef struct cache{
    icl_hash_t * files_hash_table;  //tabella hash associata alla cache 
    int max_conn;                   //numero massimo di connessioni che puo gestire la cache 
    int max_mem;                    //massima memoria occupabile
    int max_files;                  //numero massimo di file
    int occupied_memory;            //memoria attualmente occupata in cache
    pthread_mutex_t cache_mutex; //TODO forse * ?
    conc_queue * filenamequeue;     //lista dei nomi dei file nella cache,per farla fifo 
    
}cache;

/*#####################  Funzioni  ####################*/
cache * create_cache();