#include <stdio.h>
#include <stdlib.h>
#include<stdint.h>
#include "../Headers/server_globals.h"
#include "../Headers/cache.h"

/*#####################  Prototipi ########################*/


cache * create_cache();
myfile * createFile(char * filename,int clientfd,int LOCK);
int initmylock(mylock * lock);
int OpenCachedFile( cache * cache,char * filename,int clientfd,int o_create,int o_lock);
int writeFile(cache * cache,char * filename,int clientfd,int sizetowrite,char * content,char * expulsion_directory);


/*#####################  Funzioni ########################*/
static int str_ptr_cmp(const void* s, const void* t) {
    // warning: segfault if one of the pointers is null
    return strcmp((char*)s, (char*)t);
}

myfile * createFile(char * filename,int clientfd,int lOCK){
    myfile* newfile = (myfile*) malloc(sizeof(myfile));
        if(newfile == NULL)
            return NULL;
        newfile->filename = (char*)malloc(strlen(filename)*sizeof(char)+1);//alloco la stringa per contenere il nome del file
        if(newfile->filename == NULL)
            return NULL;
        strcpy(newfile->filename, filename);//copio la stringa
        newfile->content=NULL; // e inizializzo le altre variabili
        newfile->size=0;
        if(LOCK==1){
            newfile->wholocked=clientfd;
        }
        else newfile->wholocked=-1;

        /*inizializzo le struct */
        initmylock(&(newfile->mutex));
        newfile->who_opened=ll_create();
        return newfile;
}       

/* Funzione per inizializzare struttura mylock */
int initmylock(mylock * lock){
    if(pthread_mutex_init(&(lock->mutex),NULL)!=0){
        perror("inizializzando mutex mylock\n");
        exit(EXIT_FAILURE);
    }
    if(pthread_cond_init(&(lock->condition),NULL)!=0){
        perror("inizializzando mutex mylock\n");
        exit(EXIT_FAILURE);
    }
}


/* Funzione per creare ed inizializzare una cache */
cache * create_cache(){
    
    cache * ret_cache=(cache*)malloc(sizeof(cache));

    /* Informazioni della cache */
    ret_cache->max_mem=memory_dimension;                    //massima memoria occupabile
    ret_cache->max_files=num_max_file;                  //numero massimo di file
    ret_cache->occupied_memory=0;
    ret_cache->max_conn=num_workers;
    /* Creazione della lista di Nomi per gestione FIFO*/
    pthread_mutex_init(&(ret_cache->cache_mutex),NULL);
    ret_cache->filenamequeue=conc_queue_create(NULL); //TODO vedere se dichiarazione cosi va bene

    /* Creazione Tabella hash per i file */
    ret_cache->files_hash_table=icl_hash_create( (int)num_max_file, hash_pjw, string_compare );
    return ret_cache;
}               



/* Funzione per aprire un file contenuto nella cache,
se O_CREATE il file se non esistente viene creato,
se O_LOCK viene settata la lock sul file e memorizzato il fd di chi ha lockato*/
int OpenCachedFile( cache * o_cache,char * filename,int clientfd,int CREATE,int LOCK){
    if(CREATE==1){
        /*lock cache*/
        m_lock(&(o_cache->cache_mutex));
        /* cerco il file nella cache */
        if ( (icl_hash_find(o_cache->files_hash_table,filename) )!= NULL){
            if (m_unlock(&(o_cache->cache_mutex)) != 0)// se fallisco rilascio la lock sulla cache
                return FATAL_ERROR;
            errno = EEXIST;
            return ERROR;
        }
        /* Creazione nuovo file */
        myfile* newfile=createFile(filename,clientfd,LOCK);
        ll_insert_head(&(newfile->who_opened),(void*)(uintptr_t)clientfd,string_compare);//TODO forse null come argomento non  va bene
        generic_node_t * node=newfile->who_opened->head;
        while(node!=NULL){
            printf("print list %d\n",(int)(uintptr_t)node->data);
            node=node->next;
        }
        /* Aggiungo il file creato alla cache */
        icl_hash_insert(o_cache->files_hash_table,(void*)filename,(void*)newfile);
        if (m_unlock(&(o_cache->cache_mutex)) != 1)//rilascio la lock sulla cache
            return FATAL_ERROR;
        return SUCCESS;
    }
    else{
        /* Se non devo creare il file */
        m_lock(&(o_cache->cache_mutex));
        myfile * file_to_open=NULL;
        /* Cerco il file*/
        if ((file_to_open=icl_hash_find(o_cache->files_hash_table, (void*)filename)) == NULL){//cerco il file nella cache,se non trovo:
            if (m_unlock(&(o_cache->cache_mutex)) != 0)//rilascio la lock sulla cache
                return FATAL_ERROR;
            errno = ENOENT;
            return ERROR;
        }

        m_lock(&(file_to_open->mutex.mutex));
        m_unlock(&(o_cache->cache_mutex));

        /*Se devo acquisire la lock ed e libera */
        if(LOCK && file_to_open->wholocked==-1){
            file_to_open->wholocked=clientfd;
        }/*altrimenti se non la posso acquisire*/
        else if(LOCK && file_to_open->wholocked!=-1){
            m_unlock(&(file_to_open->mutex.mutex));
            perror("impossibile acquisire la lock sul file");
            return ERROR;
        }

        /* inserisco il client tra quelli che hanno aperto il file*/
        ll_insert_head(&(file_to_open->who_opened),(void*)(uintptr_t)clientfd,str_ptr_cmp);//TODO forse null come argomento non  va bene
        generic_node_t * node=file_to_open->who_opened->head;
        while(node!=NULL){
            printf("print list %d\n",(int)(uintptr_t)node->data);
            node=node->next;
        }
        m_unlock(&(file_to_open->mutex.mutex));
        return SUCCESS;
    }
}


int writeFile(cache * o_cache,char * filename,int clientfd,int sizetowrite,char * content,char * expulsion_directory){
    if(filename==NULL || clientfd <0 ||  sizetowrite<=0 || content==NULL){
        perror("dati  write file non validi\n");
        return -1;
    }
    m_lock(&(o_cache->cache_mutex));


}


