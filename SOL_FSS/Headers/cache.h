#include<pthread.h>

/*################## Structs ###################*/
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

/* lock con variabile condizione associata */
typedef struct mylock{
    pthread_mutex_t mutex; //lock
    pthread_cond_t condition; //variabile condizione
}mylock;


/*struttura del file */
typedef struct myfile{
    char* filename; //nome
    int size;       //dimensione del file
    void* content;  //contenuto
    int wholocked;  //file descriptor di chi detiene la lock
    mylock mutex;   //struttura lock
}myfile;


/* elemento della tabella hash */
typedef struct hash_element {
    void* key;
    void *data; //puntatore al file
    struct hash_element* next;
} hash_element;


/* tabella hash */
typedef struct hash_t {
    int dimension;
    hash_element ** h_element;
    /*unsigned int (*hash_function)(void*);
    int (*hash_key_compare)(void*, void*);*/
}hash_t;


/*cache */
typedef struct cache{
    hash_t * hashtable; //tabella hash associata alla cache 
    int max_conn;       //numero massimo di connessioni che puo gestire la cache 
    int max_mem;        //massima memoria occupabile
    int max_files;      
    int occupied_memory;
    mylock cache_mutex;//TODO forse * ?
    
}cache;

