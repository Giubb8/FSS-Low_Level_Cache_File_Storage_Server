#include <stdio.h>
#include <stdlib.h>
#include<stdint.h>
#include "../Headers/server_globals.h"
#include "../Headers/DataStructures/cache.h"

/*#####################  Prototipi ########################*/


cache * create_cache();
myfile * createFile(char * filename,int clientfd,int LOCK);
int initmylock(mylock * lock);
int OpenCachedFile( cache * cache,char * filename,int clientfd,int o_create,int o_lock);
int writeFile(cache * cache,char * filename,int clientfd,int sizetowrite,char * content,char * expulsion_directory);


/*#####################  Funzioni di Supporto ########################*/
static int str_ptr_cmp(const void* s, const void* t) {
    // warning: segfault if one of the pointers is null
    return strcmp((char*)s, (char*)t);
}

// helper function: compares two void pointers as integers
static int int_compare(const void* x, const void* y) {
    static int n=0;
    n++;
    printf("n: %d x: %d y: %d\n",n,*(int*)x,*(int*)y);
    // warning: segfault if one of the pointers is null
    if((*(int*)x)==(*(int*)y)) return 0;    // x=y
    else if((*(int*)x)>(*(int*)y)) return 1;    // x>y
    else return -1;      // x<y
}


myfile * createFile(char * filename,int clientfd,int lOCK){
    myfile* newfile = (myfile*) malloc(sizeof(myfile));
        if(newfile == NULL)
            return NULL;
        newfile->filename = (char*)malloc(strlen(filename)*sizeof(char)+1);//alloco la stringa per contenere il nome del file
        if(newfile->filename == NULL)
            return NULL;
        strcpy(newfile->filename, filename);//copio la stringa
        newfile->content=malloc(sizeof(char)); // e inizializzo le altre variabili
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

void destroyfile(myfile * file){

    free(file->filename);
    free(file->content);
    pthread_mutex_destroy(&(file->mutex.mutex));
    pthread_cond_destroy(&(file->mutex.condition));
    free(file);
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
    fflush(stdout);
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



/*#####################  Funzioni per Esaudire Richieste del Client ########################*/



/* Funzione per aprire un file contenuto nella cache,
se O_CREATE il file se non esistente viene creato,
se O_LOCK viene settata la lock sul file e memorizzato il fd di chi ha lockato*/
//TODO INSERIRE CONTROLLI NUMERO MASSIMO FILE
int OpenCachedFile( cache * o_cache,char * filename,int clientfd,int CREATE,int LOCK){
    if(CREATE==1){

        /* Controllo sul numero di file */
        if(o_cache->files_hash_table->nentries>=o_cache->max_files){
            perror(" Numero di Massimo di File Raggiunto\n");
            return FATAL_ERROR;
        }

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
        int * clientfd_to_pass=malloc(sizeof(int));
        *clientfd_to_pass=clientfd;
        int err=ll_insert_head(&(newfile->who_opened),(void*)clientfd_to_pass,int_compare);//TODO forse null come argomento non  va bene
        
        conc_node aux1=(newfile->who_opened)->head;
        while(aux1!=NULL){
            printf("DATA: %d\n",*(int*)aux1->data);
            aux1=aux1->next;
        }
        
        
        
        generic_node_t * node=newfile->who_opened->head;
        while(node!=NULL){
            printf("print list %d\n",(int)(uintptr_t)node->data);
            node=node->next;
        }

        /* Aggiungo il file creato alla cache */
        icl_hash_insert(o_cache->files_hash_table,(void*)filename,(void*)newfile);
        conc_queue_push((o_cache->filenamequeue),(void*)(filename));
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
            return ERROR;//TODO CAMBIARE QUESTO ERRORE CON UN ALTRO CHE DICA AL CLIENT DI RIPROVARE
        }
        
        /* inserisco il client tra quelli che hanno aperto il file*/
        int * clientfd_to_pass=malloc(sizeof(int));
        *clientfd_to_pass=clientfd;
        void * clientvoid=(void*)clientfd_to_pass;
        printf("clientfd*: %d\n",*(int*)clientvoid);
        int err=ll_insert_head(&(file_to_open->who_opened),(void*)clientfd_to_pass,int_compare);//TODO forse null come argomento non  va bene
        
        conc_node aux1=(file_to_open->who_opened)->head;
        while(aux1!=NULL){
            printf("DATA: %d\n",*(int*)aux1->data);
            aux1=aux1->next;
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

int AppendTo( cache * cache,char * filename,int clientfd,char * content){//TODO FARE CONTROLLO LOCK E RESTITUIRE ERRORE EBUSY
    fflush(stdout);
    printf("dentro funzione append %s\n",filename);
    myfile * file_to_append;
    myfile * file_to_erase;
    int returnfile=0;


    /* Se provoca capacity miss */
    if( (cache->occupied_memory+strlen(content)>(cache->max_mem)) ){        /*&& ( (strlen(content)+strlen((char*)file_to_appendd->content))<MAXCONTENT)*/ 
        /* Espelle il file con politica FIFO */
        
        returnfile=1;//segnalo che dopo dovro liberare il file
        
        m_lock(&(cache->filenamequeue->queue_mtx));
        char * filename_to_erase=conc_queue_pop(cache->filenamequeue);
        m_unlock(&(cache->filenamequeue->queue_mtx));

        m_lock(&(cache->cache_mutex));
        file_to_erase=icl_hash_find(cache->files_hash_table,filename_to_erase);
        icl_hash_delete(cache->files_hash_table,file_to_erase,free,(void*)destroyfile);
        m_unlock(&(cache->cache_mutex));
        cache->occupied_memory-=strlen(filename_to_erase);
        
    }


    m_lock(&(cache->cache_mutex));
    /* Cerco Myfile su cui fare la scrittura,effettuo controlli e scrivo */
    file_to_append=icl_hash_find(cache->files_hash_table,filename);
    
    /* Se il client ha la lock sul file o se nessuno ha la lock sul file */
    if(file_to_append->wholocked==clientfd || file_to_append->wholocked==-1){
        cache->occupied_memory+=strlen(file_to_append->content);   
        m_lock(&(file_to_append->mutex.mutex));
        m_unlock(&(cache->cache_mutex));

        if(file_to_append==NULL)printf("file null");//TODO CONTROLLO FORSE DA ELEMINARE VEDERE POI COME SI COMPORTA CON ALTRI CLIENT CONNESSI
        file_to_append->content=realloc(file_to_append->content,( (strlen((char*)file_to_append->content) + strlen((char*)content)) * sizeof(char) +1));//alloco la memoria per il contenuto da scrivere
        strcat((char *)file_to_append->content,content);
        printf("CONTENT %s\n",file_to_append->content);
        fflush(stdout);

        m_unlock(&(file_to_append->mutex.mutex));

        /* Se ho un file da rispedire indietro al client */
        if(returnfile==1){
            int content_len=strlen(content);
            content_len++;
            int filename_len=strlen(filename);
            filename_len++;
            printf("content_len: %d  filename_len: %d",content_len,filename_len);
            writen(clientfd,&content_len,sizeof(int));
            writen(clientfd,&filename_len,sizeof(int));
            writen(clientfd,filename,filename_len);
            writen(clientfd,content,content_len);

        }
        else{
            int ret=0;
            writen(clientfd,&ret,sizeof(int));
        }
        return SUCCESS;

    }
    else{
        printf("Client ha provato a modificare un file su cui non possiede la lock\n");
        return ERROR;
    } 
    

}


int CloseFile(cache * cache,char * filename,int sizefilename,int clientfd){
    static int n_op=1;
    if(p_flag)(printf("%s %d SERVER CLOSEFILE\n",opseparator,n_op));
    n_op++;
    /* Acquisizione del File */
    m_lock(&(cache->cache_mutex));
    myfile * file_to_close=NULL; 
    /* Cerco il file*/
    if ((file_to_close=icl_hash_find(cache->files_hash_table, (void*)filename)) == NULL){//cerco il file nella cache,se non trovo:
        if (m_unlock(&(cache->cache_mutex)) != 0)//rilascio la lock sulla cache
            return FATAL_ERROR;
        errno = ENOENT;
        return ERROR;
    }
    m_lock(&(file_to_close->mutex.mutex));
    m_unlock(&(cache->cache_mutex));

    /* Preparazione per la rimozione del file */
    int * clientfd_to_pass=malloc(sizeof(int));
    *clientfd_to_pass=clientfd;
    void * clientvoid=(void*)clientfd_to_pass;
    
    /* Rimozione del File */
    int err=ll_remove(&(file_to_close->who_opened),(void*)clientfd_to_pass,int_compare);

    /* Se il Client ha anche la lock la elimino */
    if(file_to_close->wholocked==clientfd){
        file_to_close->wholocked=-1;
    }
    m_unlock(&(file_to_close->mutex.mutex));

    if(err==SUCCESS){
        if(p_flag)printf("Client %d Ha Chiuso file %s\n",clientfd,filename);
        if(p_flag)printf("%s\n",opseparator);
    }
    return err;

}

LockFile(cache * cache,char * filename,int clientfd){
    static int n_op=1;
    if(p_flag)(printf("%s %d SERVER LOCKFILE %s\n",opseparator,n_op,filename));
    n_op++;
    m_lock(&(cache->cache_mutex));
    myfile * file_to_lock;
    if ((file_to_lock=icl_hash_find(cache->files_hash_table, (void*)filename)) == NULL){//cerco il file nella cache,se non trovo:
        if (m_unlock(&(cache->cache_mutex)) != 0)//rilascio la lock sulla cache
            return FATAL_ERROR;
        errno = ENOENT;
        return ERROR;
    }
    m_lock(&(file_to_lock->mutex.mutex));
    m_unlock(&(cache->cache_mutex));
    if(file_to_lock->wholocked==-1 ||file_to_lock->wholocked==clientfd ){
        file_to_lock->wholocked=clientfd;
        printf("LOCK RIUSCITA filelock=%d\n%s\n",file_to_lock->wholocked,opseparator);
        m_unlock(&(file_to_lock->mutex.mutex));
        return SUCCESS;
    }
    else{
        printf("LOCK NON RIUSCITA\n%s\n",opseparator);
        m_unlock(&(file_to_lock->mutex.mutex));
        return EBUSY;
    }
}

UnlockFile(cache * cache,char * filename,int clientfd){
    static int n_op=1;
    if(p_flag)(printf("%s %d SERVER UNLOCKFILE\n",opseparator,n_op));
    n_op++;
    m_lock(&(cache->cache_mutex));
    myfile * file_to_lock;
    if ((file_to_lock=icl_hash_find(cache->files_hash_table, (void*)filename)) == NULL){//cerco il file nella cache,se non trovo:
        if (m_unlock(&(cache->cache_mutex)) != 0)//rilascio la lock sulla cache
            return FATAL_ERROR;
        errno = ENOENT;
        return ERROR;
    }
    m_lock(&(file_to_lock->mutex.mutex));
    m_unlock(&(cache->cache_mutex));
    if(file_to_lock->wholocked==clientfd ){
        file_to_lock->wholocked=-1;
        printf("UNLOCK RIUSCITA filelock=%d\n%s\n",file_to_lock->wholocked,opseparator);
        m_unlock(&(file_to_lock->mutex.mutex));
        return SUCCESS;

    }
    else{
        printf("UNLOCK NON RIUSCITA\n%s\n",opseparator);
        m_unlock(&(file_to_lock->mutex.mutex));
        return ERROR;
    }
}

ReadFile(cache *cache,char * filename,int clientfd){
    static int n_op=0;
    n_op++;
    if(p_flag)(printf("%s %d SERVER READFILE\n",opseparator,n_op));
    myfile * file_to_read;
    int ret=1;
    m_lock(&(cache->cache_mutex));
    if ((file_to_read=icl_hash_find(cache->files_hash_table, (void*)filename)) == NULL){//cerco il file nella cache,se non trovo:
        if (m_unlock(&(cache->cache_mutex)) != 0)//rilascio la lock sulla cache
            return FATAL_ERROR;
        errno = ENOENT;
        return ERROR;
    }
    m_lock(&(file_to_read->mutex.mutex));
    m_unlock(&(cache->cache_mutex));
    int sizecontent=strlen(file_to_read->content);
    sizecontent++;
    if(sizecontent>0){   
        void * filetosend=malloc(sizecontent*(sizeof(char)));
        strcpy((char*)filetosend,(char*)file_to_read->content);
        writen(clientfd,&ret,sizeof(int));
        printf("CONTENT %s TOSEND %s %d\n",file_to_read->content,filetosend,sizecontent);
        writen(clientfd,&sizecontent,sizeof(int));
        writen(clientfd,filetosend,sizecontent);
        free(filetosend);
        return SUCCESS;
    }
    else{
        ret=0;
    }
    m_unlock(&(file_to_read->mutex.mutex));
    return SUCCESS;

}