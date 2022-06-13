#include <stdio.h>
#include <stdlib.h>
#include "../Headers/cache.h"
#include "../Headers/server_globals.h"


/* Funzione per creare una cache */
cache * create_cache(){
    
    cache * mycache=(cache*)malloc(sizeof(cache));

    /* Informazioni della cache */
    mycache->max_mem=memory_dimension;                    //massima memoria occupabile
    mycache->max_files=num_max_file;                  //numero massimo di file
    mycache->occupied_memory=0;
    mycache->max_conn=num_workers;

    /* Creazione della lista di Nomi per gestione FIFO*/
    pthread_mutex_init(&mycache->cache_mutex,NULL);
    mycache->filenamequeue=conc_queue_create(NULL); //TODO vedere se dichiarazione cosi va bene

    /* Creazione Tabella hash per i file */
    mycache->files_hash_table=icl_hash_create( (int)num_max_file, hash_pjw, string_compare );

}               


