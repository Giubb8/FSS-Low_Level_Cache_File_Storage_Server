#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include "../Headers/server_globals.h"
#include "../Headers/DataStructures/cache.h"
#include <limits.h>
/*#####################  Prototipi ########################*/

cache *create_cache();
myfile *createFile(char *filename, int clientfd, int LOCK);
int initmylock(mylock *lock);
int OpenCachedFile(cache *cache, char *filename, int clientfd, int o_create, int o_lock);
int writeFile(cache *cache, char *filename, int clientfd, int sizetowrite, char *content, char *expulsion_directory);
int ReadFile(cache *cache, char *filename, int clientfd);
int LockFile(cache *cache, char *filename, int clientfd);
int UnlockFile(cache *cache, char *filename, int clientfd);
int ReadNFile(cache *cache, int n, int clientfd);
int RemoveFile(cache *cache, char *filename, int clientfd);
int ClientDone();

/*#####################  Funzioni di Supporto ########################*/


/* Funzione per comparare due stringhe */
static int str_ptr_cmp(const void *s, const void *t){
    return strcmp((char *)s, (char *)t);
}

/*Funzione per comparare due puntatori a void come interi*/
static int int_compare(const void *x, const void *y){
    /*static int n = 0;
    n++;*/
    // printf("n: %d x: %d y: %d\n",n,*(int*)x,*(int*)y);
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
    ret_cache->max_mem = memory_dimension;  // massima memoria occupabile
    ret_cache->max_files = num_max_file;    // numero massimo di file
    ret_cache->occupied_memory = 0;         // memoria occupata 
    ret_cache->num_files = 0;               // numero di file dentro la cache 
    ret_cache->max_conn = num_workers;      // numero massimo di connessioni temporanee

    /* Creazione della lista di Nomi per gestione FIFO*/
    pthread_mutex_init(&(ret_cache->cache_mutex), NULL);
    ret_cache->filenamequeue = conc_queue_create(NULL); 

    /* Creazione Tabella hash per i file */
    ret_cache->files_hash_table = icl_hash_create((int)num_max_file, hash_pjw, string_compare);

    /* Allocazione dell'array per le statistiche */
   /* ret_cache->threadstat_array=malloc(num_workers*sizeof(threadstat));
    for(int i=0;i<num_workers;i++){
        ret_cache->threadstat_array[i].num_op=0;
        ret_cache->threadstat_array[i].thread_id=-1;
        printf("threadstat[%d] numop:%d threadid:%d\n",i,ret_cache->threadstat_array[i].num_op,ret_cache->threadstat_array[i].thread_id);
    }*/
    return ret_cache;
}

/*#####################  Funzioni per Esaudire Richieste del Client ########################*/

/* Funzione per aprire un file contenuto nella cache,
se O_CREATE il file se non esistente viene creato,
se O_LOCK viene settata la lock sul file e memorizzato il fd di chi ha lockato*/

int OpenCachedFile(cache *cache, char *filename, int clientfd, int CREATE, int LOCK){
    /*for(int i=0;i<cache->files_hash_table->nbuckets;i++){
        if(cache->files_hash_table->buckets[i]!=NULL){
            printf("bucket[%d] key: %s data:%s\n",i,cache->files_hash_table->buckets[i]->key,(char*)((myfile*)(cache->files_hash_table->buckets[i]->data))->content);
        }
        else{
            printf("bucket[%d] vuoto\n",i);
        }
    }*/
    int replace=0;
    if (CREATE == 1){

        /* Controllo sul numero di file *///TODO rimpiazzamento mettere una lock 
        if (cache->files_hash_table->nentries >= cache->max_files){
            replace=1;/*Dico al Client che non ci sara un rimpiazzamento se non c'è gia stato*/
       
            int content_len=0;
            int filename_len=0;
            myfile *file_to_erase = NULL;

            /*Dico al Client che ci sara un rimpiazzamento */
            printf("replace: %d\n",replace);
            writen(clientfd, &replace, sizeof(int));

            /* Prendo il nome del File scelto da Sostituire */
            m_lock(&(cache->filenamequeue->queue_mtx));
            char *filename_to_erase =NULL;
            //printf("DEBUG head filenamequeue %s\n",(char*)cache->filenamequeue->head->next->data);
            filename_to_erase= (char*)conc_queue_pop(cache->filenamequeue);
            m_unlock(&(cache->filenamequeue->queue_mtx));
            printf("filename to erase %s\n",filename_to_erase);
            /* Prendo il file scelto */
            m_lock(&(cache->cache_mutex));
            if ((file_to_erase = icl_hash_find(cache->files_hash_table, (void *)filename_to_erase)) == NULL){                                             // cerco il file nella cache,se non trovo:
                if (m_unlock(&(cache->cache_mutex)) != 0) // rilascio la lock sulla cache
                    return FATAL_ERROR;
                errno = ENOENT;
                return ERROR;
            }
            fflush(stdout);
            printf("filetoerase: name: %s content: %s \n",file_to_erase->filename,file_to_erase->content);
            /* Aggiorno i valori della cache ed elimino il file dalla cache */
            cache->occupied_memory -= strlen(filename_to_erase);
            cache->occupied_memory -= strlen(file_to_erase->content);
            cache->num_files--;
            icl_hash_delete(cache->files_hash_table, file_to_erase, free, (void *)destroyfile);
            m_unlock(&(cache->cache_mutex));
            


            /* Invio i dati del file */
            content_len= strlen(file_to_erase->content);
            content_len++;
            filename_len = strlen(file_to_erase->filename);
            filename_len++;
            printf("content_len: %d  filename_len: %d", content_len, filename_len);
            writen(clientfd, &content_len, sizeof(int));
            writen(clientfd, &filename_len, sizeof(int));
            writen(clientfd, file_to_erase->filename, filename_len);
            writen(clientfd, file_to_erase->content, content_len);
            m_lock(&tolog_struct_mutex);
            tolog.replaceop_tolog++;
            m_unlock(&tolog_struct_mutex);

            if(filename_to_erase) free(filename_to_erase);

        }else{
           printf("replace: %d\n",replace);
           writen(clientfd, &replace, sizeof(int)); 
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

        /* Stampa della Lista dei Client che hanno aperto il file */
        conc_node aux1 = (newfile->who_opened)->head;
        while (aux1 != NULL){
            printf("DATA: %d\n", *(int *)aux1->data);
            aux1 = aux1->next;
        }
        /*char * filenamedup=strdup(filename);
        printf("filenamedup %s\n",filenamedup);*/
        int filenamelen=strlen(filename);
        char *filenamedup=malloc(filenamelen*sizeof(char));
        strncpy(filenamedup,filename,strlen(filename));

        /* Aggiungo il file creato alla cache */
        icl_hash_insert(cache->files_hash_table, (void *)filenamedup, (void *)newfile);
        printf("filenamedup to push %s\n",filenamedup);
        /* Aggiungo il nome del file alla coda dei file della cache  */
        m_lock(&(cache->filenamequeue->queue_mtx));
        int retpush=conc_queue_push((cache->filenamequeue), (void *)filenamedup);
        printf("retpush %d  head %s\n",retpush,cache->filenamequeue->head->next->data);
        
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
        //if(filenamedup) free(filenamedup);
        int ret=SUCCESS;
        return ret;
    }
    else{
        /*Avviso che non ci puo essere un rimpiazzamento */
        writen(clientfd, &replace, sizeof(int));

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
        int *clientfd_to_pass = malloc(sizeof(int));//TODO liberare questa memoria,edit:fatto dopo controllare che vada bene
        *clientfd_to_pass = clientfd;
        int err = ll_insert_head(&(file_to_open->who_opened), (void *)clientfd_to_pass, int_compare);
        /* Stampa della Lista dei Client che hanno aperto il file */
        conc_node aux1 = (file_to_open->who_opened)->head;
        while (aux1 != NULL){
            printf("DATA: %d\n", *(int *)aux1->data);
            aux1 = aux1->next;
        }
        //free(clientfd_to_pass);

        m_unlock(&(file_to_open->mutex.mutex));
        log_op("THREAD: %d CLIENT: %d OP: OPEN FILE: %s FLAG: UNLOCKED\n",(int)syscall(__NR_gettid),clientfd,filename);        

        return SUCCESS;
    }
}

int writeFile(cache *cache, char *filename, int clientfd, int sizetowrite, char *content, char *expulsion_directory){
    if (filename == NULL || clientfd < 0 || sizetowrite <= 0 || content == NULL){
        perror("dati  write file non validi\n");
        return -1;
    }
    m_lock(&(cache->cache_mutex));
}


/* Funzione per Scrivere in Append sul File indicato */
int AppendTo(cache *cache, char *filename, int clientfd, char *content){ // TODO FARE CONTROLLO LOCK E RESTITUIRE ERRORE EBUSY
    fflush(stdout);
    myfile *file_to_append = NULL;
    myfile *file_to_erase = NULL;
    int returnfile=0;
    int content_len=0;
    int filename_len=0;
    printf("APPEND TO FILENAME %s\n", filename);

    /* Se provoca capacity miss */
    if ((cache->occupied_memory + strlen(content) > (cache->max_mem))){ /*&& ( (strlen(content)+strlen((char*)file_to_appendd->content))<MAXCONTENT)*/
        /* Espelle il file con politica FIFO */

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
        cache->occupied_memory -= strlen(filename_to_erase);
        cache->occupied_memory -= strlen(file_to_erase->content);
        cache->num_files--;
        icl_hash_delete(cache->files_hash_table, file_to_erase, free, (void *)destroyfile);
        m_unlock(&(cache->cache_mutex));
    }
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
        cache->occupied_memory += strlen(file_to_append->content);
        m_lock(&tolog_struct_mutex);
        if(cache->occupied_memory>tolog.maxcapacity_tolog)
            tolog.maxcapacity_tolog=cache->occupied_memory;
        m_unlock(&tolog_struct_mutex);

        m_lock(&(file_to_append->mutex.mutex));
        m_unlock(&(cache->cache_mutex));  
        /* Rialloco lo spazio del content e scrivo in append il nuovo content*/   
        file_to_append->content = realloc(file_to_append->content, ((strlen((char *)file_to_append->content) + strlen((char *)content)) * sizeof(char) + 1)); // alloco la memoria per il contenuto da scrivere
        strcat((char *)file_to_append->content, content);
        printf("Append, scritto sul file: %s il contenuto:\n%s\n", (char *)file_to_append->filename, (char *)file_to_append->content);
        fflush(stdout);

        m_unlock(&(file_to_append->mutex.mutex));

        /*Scrivo se debba esserci un rimpiazzamento o meno */
        writen(clientfd,&returnfile,sizeof(int));

        printf("SERVER APPENDFILE RETURNFILE %d\n",returnfile);
        /* Se ho un file da rispedire indietro al client */
        if (returnfile == 1){
            content_len= strlen(file_to_erase->content);
            content_len++;
            filename_len = strlen(file_to_erase->filename);
            filename_len++;
            printf("content_len: %d  filename_len: %d", content_len, filename_len);
            writen(clientfd, &content_len, sizeof(int));
            writen(clientfd, &filename_len, sizeof(int));
            writen(clientfd, file_to_erase->filename, filename_len);
            writen(clientfd, file_to_erase->content, content_len);
            m_lock(&tolog_struct_mutex);
            tolog.replaceop_tolog++;
            m_unlock(&tolog_struct_mutex);

        }
    
        log_op("THREAD: %d CLIENT: %d OP: APPEND WRITTEN: %d FILE: %s\n",(int)syscall(__NR_gettid),clientfd,(int)strlen(file_to_append->content),filename);
        return SUCCESS;
    }
    else{
        printf("Client ha provato a modificare un file su cui non possiede la lock\n");
        log_op("THREAD: %d CLIENT: %d OP: APPEND WRITTEN %d BYTE IN %s ERROR: NO LOCK ACQUIRED\n",(int)syscall(__NR_gettid),clientfd,content_len,filename);

        return ERROR;
    }
}

/* Funzione per chiudere il File per un Client */
int CloseFile(cache *cache, char *filename, int sizefilename, int clientfd){
    static int n_op = 1;
    if (p_flag)
        (printf("%s %d SERVER CLOSEFILE\n", opseparator, n_op));
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
        printf("Client: %d non possiede permessi per accedere al file,detenuti da %d\n", clientfd, file_to_close->wholocked);
        m_unlock(&(cache->cache_mutex));
        errno = EBUSY;
        return EBUSY;
    }
    m_lock(&(file_to_close->mutex.mutex));
    m_unlock(&(cache->cache_mutex));

    /* Preparazione per la rimozione del file */
    /*int *clientfd_to_pass = malloc(sizeof(int));
    *clientfd_to_pass = clientfd;*/
    int clientfd_to_pass=clientfd;
    /* Rimozione del Client dalla lista  */
    int err = ll_remove(&(file_to_close->who_opened), (void *)&clientfd_to_pass, int_compare);
    //free(clientfd_to_pass);
    /*if(clientfd_to_pass)
        free(clientfd_to_pass);*/
    /* Se il Client ha anche la lock la elimino */
    if (file_to_close->wholocked == clientfd){
        file_to_close->wholocked = -1;
    }
    m_unlock(&(file_to_close->mutex.mutex));

    if (err == SUCCESS){
        if (p_flag)
            printf("Client %d Ha Chiuso file %s\n", clientfd, filename);
        if (p_flag)
            printf("%s\n", opseparator);
    }
    log_op("THREAD: %d CLIENT: %d OP: CLOSE FILE: %s\n",(int)syscall(__NR_gettid),clientfd,filename);

    return err;
}

/* Funzione che locka il filename per clientfd */
int LockFile(cache *cache, char *filename, int clientfd){
    static int n_op = 1;
    if (p_flag)
        (printf("%s %d SERVER LOCKFILE %s\n", opseparator, n_op, filename));
    n_op++;
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
    if (file_to_lock->wholocked == -1 || file_to_lock->wholocked == clientfd){
        file_to_lock->wholocked = clientfd;
        printf("LOCK RIUSCITA filelock=%d\n%s\n", file_to_lock->wholocked, opseparator);
        m_unlock(&(file_to_lock->mutex.mutex));
        log_op("THREAD: %d CLIENT: %d OP: LOCK FILE: %s ERROR:NONE\n",(int)syscall(__NR_gettid),clientfd,filename);
        return SUCCESS;
    }
    else{
        printf("LOCK NON RIUSCITA\n%s\n", opseparator);
        m_unlock(&(file_to_lock->mutex.mutex));
        log_op("THREAD: %d CLIENT: %d OP: LOCK FILE: %s ERROR: FILE GIA LOCKATO\n",(int)syscall(__NR_gettid),clientfd,filename);

        return EBUSY;
    }
}

/* Funzione che Unlocka filename per clientfd */
int UnlockFile(cache *cache, char *filename, int clientfd){
    static int n_op = 1;
    if (p_flag)
        (printf("%s %d SERVER UNLOCKFILE\n", opseparator, n_op));
    n_op++;
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
    if (file_to_lock->wholocked == clientfd){
        file_to_lock->wholocked = -1;
        printf("UNLOCK RIUSCITA filelock=%d\n%s\n", file_to_lock->wholocked, opseparator);
        m_unlock(&(file_to_lock->mutex.mutex));
        log_op("THREAD: %d CLIENT: %d OP: UNLOCK FILE: %s ERROR:NONE\n",(int)syscall(__NR_gettid),clientfd,filename);

        return SUCCESS;
    }
    else{
        printf("UNLOCK NON RIUSCITA\n%s\n", opseparator);
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
    if (p_flag)
        (printf("%s %d SERVER READFILE\n", opseparator, n_op));
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
        printf("Client: %d non possiede permessi per accedere al file\n", clientfd);
        m_unlock(&(cache->cache_mutex));
        errno = EBUSY;
        return EBUSY;
    }

    m_lock(&(file_to_read->mutex.mutex));
    m_unlock(&(cache->cache_mutex));
    int sizecontent = strlen(file_to_read->content);
    if (sizecontent > 0){    
        sizecontent++;//TODO vedere se funziona
        void *filetosend = malloc(sizecontent * (sizeof(char)));
        strcpy((char *)filetosend, (char *)file_to_read->content);
        int resultwr=writen(clientfd, &ret, sizeof(int));
        printf("RET %d CONTENT %sTOSEND %s sizecontent %d resultwtr %d\n",ret, (char *)file_to_read->content, (char *)filetosend, sizecontent,resultwr);
        writen(clientfd, &sizecontent, sizeof(int));
        writen(clientfd, filetosend, sizecontent);
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
    if (p_flag)
        (printf("%s %d SERVER READ_N_FILE\n\n", opseparator, n_op));
    myfile *file_to_read;
    int sended = 0;
    int tosend;
    int i;

    /* Se la N ricevuta è <= 0 setto per mandare tutti i file della cache */
    if (n<=0 || n>cache->num_files)
        tosend = cache->num_files;
    else//altrimenti setto il numero giusto 
        tosend = n;
    
    m_lock(&(cache->cache_mutex));
    /* Stampa della Lista dei Client che hanno aperto il file */
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
            printf("File: %s\nContent: %s\n\n",file->filename, (char*)file->content );
            writen(clientfd, &contentsize, sizeof(int));
            writen(clientfd, &filenamesize, sizeof(int));
            writen(clientfd, file->filename, filenamesize);
            writen(clientfd, file->content, contentsize);
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
    printf("FILE TO REMOVE %s %s\n",file_to_remove->filename,(char*)file_to_remove->content);
    m_lock(&(file_to_remove->mutex.mutex));

    /* Controllo sulla lock del client */
    if (file_to_remove->wholocked != clientfd && file_to_remove->wholocked != -1){
        printf("Client: %d non possiede permessi per accedere al file,detenuti da %d\n", clientfd, file_to_remove->wholocked);
        m_unlock(&(cache->cache_mutex));
        m_unlock(&(file_to_remove->mutex.mutex));
        errno = EBUSY;
        return EBUSY;
    }
    else{
        cache->occupied_memory -= strlen(file_to_remove->filename);
        cache->occupied_memory -= strlen(file_to_remove->content);
        cache->num_files--;
        err = icl_hash_delete(cache->files_hash_table, file_to_remove->filename, free, (void *)destroyfile);
        printf("err %d", err);

        printf("numero di file %d", cache->files_hash_table->nentries);
    }
    m_unlock(&(cache->cache_mutex));
    m_unlock(&(file_to_remove->mutex.mutex));

    if (err == SUCCESS){
        if (p_flag)
            printf("Client %d Ha Chiuso file %s\n", clientfd, filename);
        if (p_flag)
            printf("%s\n", opseparator);
    }
    log_op("THREAD: %d CLIENT: %d OP: REMOVE FILE: %s ERROR:NONE\n",(int)syscall(__NR_gettid),clientfd,filename);

    return err;
}


int destroy_cache(cache * cache){
    printf("DENTRO DESTROY CACHE \n");
    //locko
    m_lock(&(cache->cache_mutex));
    //dealloco lista nomi
    int deallocqueue=queue_dealloc_full(cache->filenamequeue);
    printf("deallocqueue %d\n",deallocqueue);
    //dealloco tabella hash
    if(cache->files_hash_table!=NULL)
        icl_hash_destroy(cache->files_hash_table, free, (void (*)(void *)) destroyfile);
    /*
    if(cache->threadstat_array)
        free(cache->threadstat_array);*/
    //unlocko
    m_unlock(&(cache->cache_mutex));
    //libero tutto il resto
    if(cache)
        free(cache);
}