#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include "../Headers/server_globals.h"
#include "../Headers/DataStructures/cache.h"
#include <limits.h>

#define B2MB 1000000
/*#####################  PROTOTIPI ########################*/

cache *create_cache();
myfile *createFile(char *filename, int clientfd, int LOCK);
int initmylock(mylock *lock);
int OpenFile(cache *cache, char *filename, int clientfd, int o_create, int o_lock);
int writeFile(cache *cache, char *filename, int clientfd, int sizetowrite, char *content, char *expulsion_directory);
int ReadFile(cache *cache, char *filename, int clientfd);
int LockFile(cache *cache, char *filename, int clientfd);
int UnlockFile(cache *cache, char *filename, int clientfd);
int ReadNFile(cache *cache, int n, int clientfd);
int RemoveFile(cache *cache, char *filename, int clientfd);
int ClientDone();

/*#####################  FUNZIONI DI SUPPORTO ########################*/


/* Funzione per comparare due stringhe */
static int str_ptr_cmp(const void *s, const void *t){
    return strcmp((char *)s, (char *)t);
}

/*Funzione per comparare due puntatori a void come interi*/
static int int_compare(const void *x, const void *y){
    /*static int n = 0;
    n++;*/
    // if(p_flag)printf("n: %d x: %d y: %d\n",n,*(int*)x,*(int*)y);
    if ((*(int *)x) == (*(int *)y))
        return 0; // x=y
    else if ((*(int *)x) > (*(int *)y))
        return 1; // x>y
    else
        return -1; // x<y
}


/* Funzione per creare un puntatore a File */
myfile *createFile(char *filename, int clientfd, int lOCK){
    myfile *newfile = (myfile *)malloc(sizeof(myfile));
    if (newfile == NULL)
        return NULL;
    newfile->filename = (char *)malloc(strlen(filename) * sizeof(char) + 1); // alloco la stringa per contenere il nome del file
    if (newfile->filename == NULL)
        return NULL;
    strcpy(newfile->filename, filename);     // copio la stringa
    newfile->content = malloc(sizeof(char)); // e inizializzo le altre variabili
    memset(newfile->content,0,sizeof(char));
    newfile->size = 0;
    
    /* Se devo acquisire la Lock */
    if (LOCK == 1){
        newfile->wholocked = clientfd;
    }
    else
        newfile->wholocked = -1;

    /*inizializzo le struct */
    initmylock(&(newfile->mutex));
    newfile->who_opened = ll_create();
    return newfile;
}

/* Funzione per distruggere myfile */
void destroyfile(myfile *file){
    free(file->filename);
    free(file->content);
    pthread_mutex_destroy(&(file->mutex.mutex));
    pthread_cond_destroy(&(file->mutex.condition));
    ll_dealloc_full(file->who_opened);
    free(file);
}

/* Funzione per inizializzare struttura mylock */
int initmylock(mylock *lock){
    if(pthread_mutex_init(&(lock->mutex), NULL) != 0){
        perror("inizializzando mutex mylock\n");
        exit(EXIT_FAILURE);
    }
    if(pthread_cond_init(&(lock->condition), NULL) != 0){
        perror("inizializzando mutex mylock\n");
        exit(EXIT_FAILURE);
    }
}

/* Funzione per creare ed inizializzare una cache */
cache *create_cache(){
    fflush(stdout);
    cache *ret_cache = (cache *)malloc(sizeof(cache));

    /* Informazioni della cache */
    ret_cache->max_mem = B2MB*memory_dimension;  // massima memoria occupabile
    ret_cache->max_files = num_max_file;    // numero massimo di file
    ret_cache->occupied_memory = 0;         // memoria occupata 
    ret_cache->num_files = 0;               // numero di file dentro la cache 
    ret_cache->max_conn = num_workers;      // numero massimo di connessioni temporanee

    /* Creazione della lista di Nomi per gestione FIFO*/
    pthread_mutex_init(&(ret_cache->cache_mutex), NULL);
    ret_cache->filenamequeue = conc_queue_create(NULL); 

    /* Creazione Tabella hash per i file */
    ret_cache->files_hash_table = icl_hash_create((int)num_max_file, hash_pjw, string_compare);

    return ret_cache;
}

/*#####################  Funzioni per Esaudire Richieste del Client ########################*/

/* Funzione per aprire un file contenuto nella cache,
se O_CREATE il file se non esistente viene creato,
se O_LOCK viene settata la lock sul file e memorizzato il fd di chi ha lockato*/

int OpenFile(cache *cache, char *filename, int clientfd, int CREATE, int LOCK){
    int replace=0;
    if (CREATE==1){

        /* Controllo sul numero di file */
        m_lock(&(cache->cache_mutex));
        if (cache->files_hash_table->nentries >= cache->max_files){
            m_unlock(&(cache->cache_mutex));

            replace=1;
       
            int content_len=0;
            int filename_len=0;
            myfile *file_to_erase = NULL;

            /*Dico al Client che ci sara un rimpiazzamento */
            d_writen(clientfd, &replace, sizeof(int));

            /* Prendo il nome del File scelto da Sostituire */
            m_lock(&(cache->filenamequeue->queue_mtx));
            char *filename_to_erase =NULL;
           
            filename_to_erase= (char*)conc_queue_pop(cache->filenamequeue);
            m_unlock(&(cache->filenamequeue->queue_mtx));
            if(p_flag)printf("filename to erase %s\n",filename_to_erase);

            /* Prendo il file scelto */
            m_lock(&(cache->cache_mutex));
            if ((file_to_erase = icl_hash_find(cache->files_hash_table, (void *)filename_to_erase)) == NULL){                                             // cerco il file nella cache,se non trovo:
                if (m_unlock(&(cache->cache_mutex)) != 0) // rilascio la lock sulla cache
                    return FATAL_ERROR;
                errno = ENOENT;
                return ERROR;
            }
            fflush(stdout);
            if(p_flag)printf("filetoerase: name: %s content: %s \n",file_to_erase->filename,(char*)file_to_erase->content);
            
            /* Aggiorno i valori della cache ed elimino il file dalla cache */
            cache->occupied_memory -= strlen(file_to_erase->content);
            if(p_flag)printf("cache occupied memory %d\n",cache->occupied_memory);
            cache->num_files--;
            icl_hash_delete(cache->files_hash_table, file_to_erase, free, (void *)destroyfile);
            m_unlock(&(cache->cache_mutex));
            


            /* Invio i dati del file */
            content_len= strlen(file_to_erase->content);
            content_len++;
            filename_len = strlen(file_to_erase->filename);
            filename_len++;
            if(p_flag)printf("content_len: %d  filename_len: %d", content_len, filename_len);
            d_writen(clientfd, &content_len, sizeof(int));
            d_writen(clientfd, &filename_len, sizeof(int));
            d_writen(clientfd, file_to_erase->filename, filename_len);
            d_writen(clientfd, file_to_erase->content, content_len);
            m_lock(&tolog_struct_mutex);
            tolog.replaceop_tolog++;
            m_unlock(&tolog_struct_mutex);

            if(filename_to_erase) free(filename_to_erase);

        }else{
            m_unlock(&(cache->cache_mutex));
            if(p_flag)printf("replace: %d\n",replace);
            d_writen(clientfd, &replace, sizeof(int)); 
        }
        

        
        /*lock cache*/
        m_lock(&(cache->cache_mutex));

        /* cerco il file nella cache */
        if ((icl_hash_find(cache->files_hash_table, filename)) != NULL){
            if (m_unlock(&(cache->cache_mutex)) != 0) // se fallisco rilascio la lock sulla cache
                return FATAL_ERROR;
            errno = EEXIST;
            return ERROR;
        }

        /* Creazione nuovo file con gestione del flag LOCK */
        myfile *newfile = createFile(filename, clientfd, LOCK);
        int *clientfd_to_pass = malloc(sizeof(int));
        *clientfd_to_pass = clientfd;
        int err = ll_insert_head(&(newfile->who_opened), (void *)clientfd_to_pass, int_compare);

        
        int filenamelen=strlen(filename);
        
        /* Aggiungo il file creato alla cache */
        icl_hash_insert(cache->files_hash_table, (void *)strdup(filename), (void *)newfile);

        /* Aggiungo il nome del file alla coda dei file della cache  */
        m_lock(&(cache->filenamequeue->queue_mtx));
        int retpush=conc_queue_push((cache->filenamequeue), strdup(filename));
        if(p_flag)printf("retpush %d  head %s\n",retpush,(char*)cache->filenamequeue->head->next->data);
        
        m_unlock(&(cache->filenamequeue->queue_mtx));

        /* Aggiorno la Statistica per il log*/                
        cache->num_files++;
        m_lock(&tolog_struct_mutex);
        if(cache->num_files>tolog.maxnumfile_tolog)
            tolog.maxnumfile_tolog=cache->num_files;
        m_unlock(&tolog_struct_mutex);

        if (m_unlock(&(cache->cache_mutex)) != 1) // rilascio la lock sulla cache
            return FATAL_ERROR;
        log_op("THREAD: %d CLIENT: %d OP: OPEN FILE: %s FLAG: LOCKED\n",(int)syscall(__NR_gettid),clientfd,filename);        
        int ret=SUCCESS;
        return ret;
    }
    else{
        /*Avviso che non ci puo essere un rimpiazzamento */
        d_writen(clientfd, &replace, sizeof(int));

        /* Se non devo creare il file */
        m_lock(&(cache->cache_mutex));
        myfile *file_to_open = NULL;

        /* Cerco il file*/
        if ((file_to_open = icl_hash_find(cache->files_hash_table, (void *)filename)) == NULL){                                               // cerco il file nella cache,se non trovo:
            if (m_unlock(&(cache->cache_mutex)) != 0) // rilascio la lock sulla cache
                return FATAL_ERROR;
            errno = ENOENT;
            return ERROR;
        }
        m_lock(&(file_to_open->mutex.mutex));
        m_unlock(&(cache->cache_mutex));

        /*Se devo acquisire la lock ed e libera */
        if (LOCK && file_to_open->wholocked == -1){
            file_to_open->wholocked = clientfd;
        } /*altrimenti se non la posso acquisire*/
        else if (LOCK && file_to_open->wholocked != -1){
            m_unlock(&(file_to_open->mutex.mutex));
            perror("impossibile acquisire la lock sul file");
            return EBUSY; 
        }

        /* inserisco il client tra quelli che hanno aperto il file*/
        int *clientfd_to_pass = malloc(sizeof(int));
        *clientfd_to_pass = clientfd;
        int err = ll_insert_head(&(file_to_open->who_opened), (void *)clientfd_to_pass, int_compare);
        

        m_unlock(&(file_to_open->mutex.mutex));
        log_op("THREAD: %d CLIENT: %d OP: OPEN FILE: %s FLAG: UNLOCKED\n",(int)syscall(__NR_gettid),clientfd,filename);        

        return SUCCESS;
    }
}


/* Funzione per Scrivere in Append sul File indicato */
int AppendTo(cache *cache, char *filename, int clientfd, char *content){ 
    fflush(stdout);
    myfile *file_to_append = NULL;
    myfile *file_to_erase = NULL;
    int returnfile=0;
    int content_len=0;
    int filename_len=0;
    if(p_flag)printf("APPEND TO FILENAME %s\n", filename);
    m_lock(&(cache->cache_mutex));
    //printf("STATISTICHE APPEND TO CACHE OCCUPIED %d, STRLEN CONTENT %d CACHE MAXMEM %d\n",cache->occupied_memory,strlen(content),cache->max_mem);
    
    /* Se provoca capacity miss */
    if ((cache->occupied_memory + strlen(content) > (cache->max_mem))){ /*&& ( (strlen(content)+strlen((char*)file_to_appendd->content))<MAXCONTENT)*/
        m_unlock(&(cache->cache_mutex));

        /* Espelle il file con politica FIFO */
        if(p_flag)printf("REPLACE APPEND occupied memory %d strlen content %d maxmem %d\n",cache->occupied_memory,(int)strlen(content),cache->max_mem);
        returnfile = 1; // segnalo che dopo dovro liberare il file
        
        /* Prendo il nome del File scelto da Sostituire */
        m_lock(&(cache->filenamequeue->queue_mtx));
        char *filename_to_erase = conc_queue_pop(cache->filenamequeue);
        m_unlock(&(cache->filenamequeue->queue_mtx));

        /* Prendo il file scelto */
        m_lock(&(cache->cache_mutex));
        if ((file_to_erase = icl_hash_find(cache->files_hash_table, (void *)filename_to_erase)) == NULL){                                             // cerco il file nella cache,se non trovo:
            if (m_unlock(&(cache->cache_mutex)) != 0) // rilascio la lock sulla cache
                return FATAL_ERROR;
            errno = ENOENT;
            return ERROR;
        }
        fflush(stdout);
        /* Aggiorno i valori della cache ed elimino il file dalla cache */
        cache->occupied_memory -= strlen(file_to_erase->content);
        if(p_flag)printf("cache occupied memory %d\n",cache->occupied_memory);

        cache->num_files--;
        icl_hash_delete(cache->files_hash_table, file_to_erase, free, (void *)destroyfile);
        m_unlock(&(cache->cache_mutex));
    }
    m_unlock(&(cache->cache_mutex));

    /* Se non provoca capacity miss */

    m_lock(&(cache->cache_mutex));
    /* Cerco il file su cui fare la scrittura */
    fflush(stdout);
    fflush(stdin);
    fflush(stderr);
    if ((file_to_append = (myfile *)icl_hash_find(cache->files_hash_table, (void *)filename)) == NULL){                                             // cerco il file nella cache,se non trovo:
        if (m_unlock(&(cache->cache_mutex)) != 0) // rilascio la lock sulla cache
            return FATAL_ERROR;
        errno = EBUSY;
        return EBUSY;
    }

    /* Se il client ha la lock sul file o se nessuno ha la lock sul file */
    if (file_to_append->wholocked == clientfd || file_to_append->wholocked == -1){
        /* Aggiornamento per logging */
        int lencontent=strlen(content);
        cache->occupied_memory += lencontent;
        if(p_flag)printf("cache occupied memory %d lencontent %d \n",cache->occupied_memory,lencontent);

        m_lock(&tolog_struct_mutex);
        if(cache->occupied_memory>tolog.maxcapacity_tolog)
            tolog.maxcapacity_tolog=cache->occupied_memory;
        m_unlock(&tolog_struct_mutex);

        m_lock(&(file_to_append->mutex.mutex));
        m_unlock(&(cache->cache_mutex));  
        /* Rialloco lo spazio del content e scrivo in append il nuovo content*/   
        file_to_append->content = realloc(file_to_append->content, ((strlen((char *)file_to_append->content) + strlen((char *)content)) * sizeof(char) + 1)); // alloco la memoria per il contenuto da scrivere
        strcat((char *)file_to_append->content, content);
        if(p_flag)printf("Append, scritto sul file: %s il contenuto:\n%s\n", (char *)file_to_append->filename, (char *)file_to_append->content);
        fflush(stdout);

        m_unlock(&(file_to_append->mutex.mutex));

        /*Scrivo se debba esserci un rimpiazzamento o meno */
        d_writen(clientfd,&returnfile,sizeof(int));

        if(p_flag)printf("SERVER APPENDFILE RETURNFILE %d\n",returnfile);
        /* Se ho un file da rispedire indietro al client */
        if (returnfile == 1){
            content_len= strlen(file_to_erase->content);
            content_len++;
            filename_len = strlen(file_to_erase->filename);
            filename_len++;
            if(p_flag)printf("content_len: %d  filename_len: %d", content_len, filename_len);
            d_writen(clientfd, &content_len, sizeof(int));
            d_writen(clientfd, &filename_len, sizeof(int));
            d_writen(clientfd, file_to_erase->filename, filename_len);
            d_writen(clientfd, file_to_erase->content, content_len);
            m_lock(&tolog_struct_mutex);
            tolog.replaceop_tolog++;
            m_unlock(&tolog_struct_mutex);

        }
    
        log_op("THREAD: %d CLIENT: %d OP: APPEND WRITTEN: %d FILE: %s\n",(int)syscall(__NR_gettid),clientfd,lencontent,filename);
        return SUCCESS;
    }
    else{
        if(p_flag)printf("Client ha provato a modificare un file su cui non possiede la lock\n");
        log_op("THREAD: %d CLIENT: %d OP: APPEND WRITTEN %d BYTE IN %s ERROR: NO LOCK ACQUIRED\n",(int)syscall(__NR_gettid),clientfd,content_len,filename);

        return ERROR;
    }
}

/* Funzione per chiudere il File per un Client */
int CloseFile(cache *cache, char *filename, int sizefilename, int clientfd){
    static int n_op = 1;
   
    if(p_flag)printf("%s %d SERVER CLOSEFILE\n", opseparator, n_op);
    n_op++;
    /* Acquisizione del File */
    m_lock(&(cache->cache_mutex));
    myfile *file_to_close = NULL;

    /* Cerco il file*/
    if ((file_to_close = icl_hash_find(cache->files_hash_table, (void *)filename)) == NULL){                                             // cerco il file nella cache,se non trovo:
        if (m_unlock(&(cache->cache_mutex)) != 0) // rilascio la lock sulla cache
            return FATAL_ERROR;
        errno = ENOENT;
        return ERROR;
    }
    /* Controllo sulla lock del client */
    if (file_to_close->wholocked != clientfd && file_to_close->wholocked != -1){
        if(p_flag)printf("Client: %d non possiede permessi per accedere al file,detenuti da %d\n", clientfd, file_to_close->wholocked);
        m_unlock(&(cache->cache_mutex));
        errno = EBUSY;
        return EBUSY;
    }
    m_lock(&(file_to_close->mutex.mutex));
    m_unlock(&(cache->cache_mutex));

    /* Preparazione per la rimozione del file */
    int clientfd_to_pass=clientfd;
    /* Rimozione del Client dalla lista  */
    int err = ll_remove(&(file_to_close->who_opened), (void *)&clientfd_to_pass, int_compare);
    if(p_flag)printf("err close file %d\n",err);
    
    /* Se il Client ha anche la lock la elimino */
    if (file_to_close->wholocked == clientfd){
        file_to_close->wholocked = -1;
    }
    m_unlock(&(file_to_close->mutex.mutex));

    if (err == SUCCESS){
        if (p_flag)printf("Client %d Ha Chiuso file %s\n", clientfd, filename);
        if (p_flag)printf("%s\n", opseparator);
    }
    log_op("THREAD: %d CLIENT: %d OP: CLOSE FILE: %s\n",(int)syscall(__NR_gettid),clientfd,filename);

    return err;
}

/* Funzione che locka il filename per clientfd */
int LockFile(cache *cache, char *filename, int clientfd){
    static int n_op = 1;
    if (p_flag)(printf("%s %d SERVER LOCKFILE %s\n", opseparator, n_op, filename));
    n_op++;

    /* Cerco il file all'interno della cache */
    m_lock(&(cache->cache_mutex));
    myfile *file_to_lock;
    if ((file_to_lock = icl_hash_find(cache->files_hash_table, (void *)filename)) == NULL){                                             // cerco il file nella cache,se non trovo:
        if (m_unlock(&(cache->cache_mutex)) != 0) // rilascio la lock sulla cache
            return FATAL_ERROR;
        errno = ENOENT;
        return ERROR;
    }
    m_lock(&(file_to_lock->mutex.mutex));
    m_unlock(&(cache->cache_mutex));
    /* Se il Client ha gia la lock o se la lock e' libera restituisco successo */
    if (file_to_lock->wholocked == -1 || file_to_lock->wholocked == clientfd){
        file_to_lock->wholocked = clientfd;
        if(p_flag)printf("LOCK RIUSCITA filelock=%d\n%s\n", file_to_lock->wholocked, opseparator);
        m_unlock(&(file_to_lock->mutex.mutex));
        log_op("THREAD: %d CLIENT: %d OP: LOCK FILE: %s ERROR:NONE\n",(int)syscall(__NR_gettid),clientfd,filename);
        return SUCCESS;
    }
    else{/* Altrimenti se non e' libera dico al client di riprovare */
        if(p_flag)printf("LOCK NON RIUSCITA\n%s\n", opseparator);
        m_unlock(&(file_to_lock->mutex.mutex));
        log_op("THREAD: %d CLIENT: %d OP: LOCK FILE: %s ERROR: FILE GIA LOCKATO\n",(int)syscall(__NR_gettid),clientfd,filename);

        return EBUSY;
    }
}

/* Funzione che Unlocka filename per clientfd */
int UnlockFile(cache *cache, char *filename, int clientfd){
    static int n_op = 1;
    if (p_flag)(printf("%s %d SERVER UNLOCKFILE\n", opseparator, n_op));
    n_op++;
    /* Cerco il file all'interno della cache */
    m_lock(&(cache->cache_mutex));
    myfile *file_to_lock;
    if ((file_to_lock = icl_hash_find(cache->files_hash_table, (void *)filename)) == NULL){                                             // cerco il file nella cache,se non trovo:
        if (m_unlock(&(cache->cache_mutex)) != 0) // rilascio la lock sulla cache
            return FATAL_ERROR;
        errno = ENOENT;
        return ERROR;
    }
    m_lock(&(file_to_lock->mutex.mutex));
    m_unlock(&(cache->cache_mutex));
    /* Se il client possiede la lock la libero */
    if (file_to_lock->wholocked == clientfd){
        file_to_lock->wholocked = -1;
        if(p_flag)printf("UNLOCK RIUSCITA filelock=%d\n%s\n", file_to_lock->wholocked, opseparator);
        m_unlock(&(file_to_lock->mutex.mutex));
        log_op("THREAD: %d CLIENT: %d OP: UNLOCK FILE: %s ERROR:NONE\n",(int)syscall(__NR_gettid),clientfd,filename);

        return SUCCESS;
    }
    else{/* Altrimenti Fallisco */
        if(p_flag)printf("UNLOCK NON RIUSCITA\n%s\n", opseparator);
        m_unlock(&(file_to_lock->mutex.mutex));
        log_op("THREAD: %d CLIENT: %d OP: UNLOCK FILE: %s ERROR: FILE GIA SENZA LOCK\n",(int)syscall(__NR_gettid),clientfd,filename);

        return ERROR;
    }
}

/* Funzione che legge un file */
int ReadFile(cache *cache, char *filename, int clientfd){
    /* Inizializzazione */
    static int n_op = 0;
    n_op++;
    if (p_flag)(printf("%s %d SERVER READFILE\n", opseparator, n_op));
    myfile *file_to_read;
    int ret = INT_MAX;

    /* Cerco il file nella tabella */
    m_lock(&(cache->cache_mutex));
    if ((file_to_read = icl_hash_find(cache->files_hash_table, (void *)filename)) == NULL){                                             // cerco il file nella cache,se non trovo:
        if (m_unlock(&(cache->cache_mutex)) != 0) // rilascio la lock sulla cache
            return FATAL_ERROR;
        errno = ENOENT;
        return ERROR;
    }

    /* Controllo sulla lock del client */
    if (file_to_read->wholocked != clientfd && file_to_read->wholocked != -1){
        if(p_flag)printf("Client: %d non possiede permessi per accedere al file\n", clientfd);
        m_unlock(&(cache->cache_mutex));
        errno = EBUSY;
        return EBUSY;
    }

    m_lock(&(file_to_read->mutex.mutex));
    m_unlock(&(cache->cache_mutex));
    int sizecontent = strlen(file_to_read->content);
    /* Se il file non e' vuoto lo leggo */
    if (sizecontent > 0){    
        sizecontent++;
        void *filetosend = malloc(sizecontent * (sizeof(char)));
        strcpy((char *)filetosend, (char *)file_to_read->content);
        int resultwr=d_writen(clientfd, &ret, sizeof(int));
        if(p_flag)printf("RET %d CONTENT %sTOSEND %s sizecontent %d resultwtr %d\n",ret, (char *)file_to_read->content, (char *)filetosend, sizecontent,resultwr);
        d_writen(clientfd, &sizecontent, sizeof(int));
        d_writen(clientfd, filetosend, sizecontent);
        free(filetosend);
        m_unlock(&(file_to_read->mutex.mutex));
        log_op("THREAD: %d CLIENT: %d OP: READ READED %d FILE: %s ERROR:NONE\n",(int)syscall(__NR_gettid),clientfd,sizecontent,filename);
        fflush(stdout);
        return SUCCESS;
    }
    else{
        m_unlock(&(file_to_read->mutex.mutex));
        log_op("THREAD: %d CLIENT: %d OP: READ READED %d FILE: %s ERROR:CONTENT VUOTO\n",(int)syscall(__NR_gettid),clientfd,sizecontent,filename);
        fflush(stdout);

        return SUCCESS;
    }
}

/* Legge N File dalla cache */
int ReadNFile(cache *cache, int n, int clientfd){
    /* Inizializzazione */
    static int n_op = 0;
    n_op++;
    if (p_flag)(printf("%s %d SERVER READ_N_FILE numfile presenti %d\n\n", opseparator, n_op,cache->num_files));
    myfile *file_to_read;
    int sended=0;
    int tosend=0;
    int i;
    
    /* Calcolo e Restituisco  al Client il numero giusto di file da leggere */
    if (n<=0 || n >cache->num_files)
        tosend = cache->num_files;
    else//altrimenti setto il numero giusto 
        tosend = n;
    if(p_flag)printf("tosend prima %d\n",tosend);
    d_writen(clientfd,&tosend,sizeof(int));

    m_lock(&(cache->cache_mutex));
    /* Cerco nella cache n file,n precedentemente calcolato, e li restituisco al client */
    icl_entry_t *bucket, *curr, *next;
    for (i = 0; i < cache->files_hash_table->nbuckets; i++){
        bucket = cache->files_hash_table->buckets[i];
        for (curr = bucket; curr != NULL;){

            if (sended == tosend)
                break;
            myfile *file;
            next = curr->next;
            file = curr->data;
            m_lock(&(file->mutex.mutex));
            int contentsize = strlen(file->content);
            contentsize++;
            int filenamesize = strlen(file->filename);
            filenamesize++;
            if(p_flag)printf("File: %s\nContent: %s\n contentsize: %d filenamesize %d\n",file->filename, (char*)file->content,contentsize,filenamesize);
            d_writen(clientfd, &contentsize, sizeof(int));
            d_writen(clientfd, &filenamesize, sizeof(int));
            d_writen(clientfd, file->filename, filenamesize);
            d_writen(clientfd, file->content, contentsize);
            m_unlock(&(file->mutex.mutex));
            curr = next;
            sended++;
            log_op("THREAD: %d CLIENT: %d OP: READNFILE READED %d FILE: %s ERROR:NONE\n",(int)syscall(__NR_gettid),clientfd,contentsize,file->filename);

        }
    }
    m_unlock(&(cache->cache_mutex));
}

/* Funzione che rimuove il file dalla cache */
int RemoveFile(cache *cache, char *filename, int clientfd){
    static int n_op = 1;
    if (p_flag)
        (printf("%s %d SERVER REMOVEFILE\n", opseparator, n_op));
    n_op++;
    int err = -1;

    /* Acquisizione del File */
    m_lock(&(cache->cache_mutex));
    myfile *file_to_remove = NULL;
    /* Cerco il file*/
    if ((file_to_remove = icl_hash_find(cache->files_hash_table, (void *)filename)) == NULL){                                             // cerco il file nella cache,se non trovo:
        if (m_unlock(&(cache->cache_mutex)) != 0) // rilascio la lock sulla cache
            return FATAL_ERROR;
        errno = ENOENT;
        return ERROR;
    }
    if(p_flag)printf("FILE TO REMOVE %s %s\n",file_to_remove->filename,(char*)file_to_remove->content);
    m_lock(&(file_to_remove->mutex.mutex));

    /* Controllo sulla lock del client */
    if (file_to_remove->wholocked != clientfd && file_to_remove->wholocked != -1){
        if(p_flag)printf("Client: %d non possiede permessi per accedere al file,detenuti da %d\n", clientfd, file_to_remove->wholocked);
        m_unlock(&(cache->cache_mutex));
        m_unlock(&(file_to_remove->mutex.mutex));
        errno = EBUSY;
        return EBUSY;
    }
    else{/* Se posso lavorare sul file lo elimino dalla cache */
        cache->occupied_memory -= strlen(file_to_remove->content);
        if(p_flag)printf("cache occupied memory %d\n",cache->occupied_memory);

        cache->num_files--;     
        m_unlock(&(file_to_remove->mutex.mutex));
        err = icl_hash_delete(cache->files_hash_table, file_to_remove->filename, free, (void *)destroyfile);
        if(p_flag)printf("err %d", err);

        if(p_flag)printf("numero di file %d", cache->files_hash_table->nentries);
    }
    m_unlock(&(cache->cache_mutex));

    if (err == SUCCESS){
        if (p_flag)printf("Client %d Ha Chiuso file %s\n", clientfd, filename);
        if (p_flag)printf("%s\n", opseparator);
    }
    log_op("THREAD: %d CLIENT: %d OP: REMOVE FILE: %s ERROR:NONE\n",(int)syscall(__NR_gettid),clientfd,filename);

    return err;
}


int destroy_cache(cache * cache){
    if(p_flag)printf("DENTRO DESTROY CACHE \n");
    //locko
    //dealloco lista nomi

    if(p_flag)printf("coda vuota %d",conc_queue_isEmpty(cache->filenamequeue));
    if(p_flag)printf("head next filenamequeue %s\n",(char*)cache->filenamequeue->head->next->data);
    if(p_flag)printf("PRIMA\n");

    /* Libero la coda dei filename */
    while(conc_queue_isEmpty(cache->filenamequeue)!=1){
        char * tmp=NULL;
        tmp=conc_queue_pop(cache->filenamequeue);
        if(p_flag)printf("tmp %s\n",tmp);
        free(tmp);
    }
    int deallocqueue=queue_dealloc_full(mycache->filenamequeue);

    if(p_flag)printf("deallocqueue %d\n",deallocqueue);
    if(p_flag)printf("DOPO\n");
    /* Deallocazione Tabella Hash */
    icl_hash_destroy(cache->files_hash_table, free, (void (*)(void *)) destroyfile);
    
    if(cache)
        free(cache);
}