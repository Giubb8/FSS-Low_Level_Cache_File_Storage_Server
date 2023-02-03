
#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<pthread.h>
#include<string.h>
#include<unistd.h>
#include<stdint.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include "../Headers/comm.h"
#include <fcntl.h>


/*####################  STRUTTURE E GLOBALS  ############################*/

/* Struttura per memorizzare i messaggi ricevuti dal client*/
conc_queue * clientsqueue=NULL; //lista dei clienti che devono essere serviti



/*####################  FUNZIONI DI UTILITY PER LE COMUNICAZIONI ############################*/
 

/* Lock con gestione errore*/
int m_lock(pthread_mutex_t* mtx){
  int error;
  if( (error=pthread_mutex_lock(mtx)) !=0) {
    errno=error;
    perror("\nLocking mutex: ");
    exit(EXIT_FAILURE);
  }
  if(comm_flag){printf("Mutex locked\n");}
  return 1;
}

/* Unlock con gestione errore*/
int m_unlock(pthread_mutex_t* mtx) {
  int error;
  if( (error=pthread_mutex_unlock(mtx))!=0) {
    errno=error;
    perror("\nUnlocking mutex: ");
    _exit(EXIT_FAILURE);
  }
  if(comm_flag){printf("Mutex unlocked\n");}
  return 1;
}

/* Wait con gestione errore */
int m_wait(pthread_cond_t* cond, pthread_mutex_t* mtx) {
  int error;
  if((error=pthread_cond_wait(cond, mtx))!=0) {
    errno=error;
    perror("\nPreparing to wait for cond: ");
    _exit(EXIT_FAILURE);
  }
  if(comm_flag){printf("Wait in corso\n");}

  return 1;
}

/* Signal con gestione errore */
int m_signal(pthread_cond_t* cond){
  int error;
  if((error=pthread_cond_signal(cond))!=0) {
    errno=error;
    perror("\nSignaling cond: ");
    _exit(EXIT_FAILURE);
  }
  if(comm_flag){printf("Signal inviata\n");}
  return 1;
}



/*####################  FUNZIONI PER LE COMUNICAZIONI E GESTIONE RICHIESTE  ############################*/

/* Gestione delle Operazioni */
int handleop(msg request,int clientfd){
    if(request.op==OPEN){
        /* Gestisco i Flag */
        int lock=0;
        int create=0;
        if(request.flag==O_BOTH){
            lock=1;
            create=1;
        }
        else if(request.flag==O_CREATE){
            create=1;
        }
        else if(request.flag==O_LOCK){
            lock=1;
        }
        else if(request.flag==NO_FLAG){
            lock=0;
            create=0;
        }
        else{
            perror("Flag non riconosciuto\n");
            exit(EXIT_FAILURE);
        }

        int ret=OpenFile(mycache,request.filepath,clientfd,create,lock);
        fflush(stdout);
        return ret;
    }
    if(request.op==APPEND){
        int ret=-1;
        ret=AppendTo(mycache,request.filepath,clientfd,request.content);
        fflush(stdout);
        return ret;
    }
    if(request.op==CLOSE){
        int ret=CloseFile(mycache,request.filepath,request.size,clientfd);
        fflush(stdout);
        return ret;
    }
    if(request.op==LOCK){
        int ret=LockFile(mycache,request.filepath,clientfd);
        fflush(stdout);
        return ret;
    }
    if(request.op==UNLOCK){
        int ret=UnlockFile(mycache,request.filepath,clientfd);
        fflush(stdout);
        return ret;
    }
    if(request.op==READ){
        int ret=ReadFile(mycache,request.filepath,clientfd);
        fflush(stdout);
        return ret;
    }
    if(request.op==READN){
        int ret=ReadNFile(mycache,request.size,clientfd);
        fflush(stdout);
        return ret;
    }
    if(request.op==REMOVE){
        int ret=RemoveFile(mycache,request.filepath,clientfd);
        fflush(stdout);
        return ret;
    }

}

/* Gestione Delle Connessioni Con il Client */
void * handleconnection(int arg){
    int ret;
    int clientfd=(int)(uintptr_t)arg;
    while (sighintquit==0){//se ricevo sighint o sigquit non devo più gestire le richieste con i client connessi  
        msg request={0};
        memset(request.filepath,0,sizeof(request.filepath));
        memset(request.content,0,sizeof(request.content));
        d_readn(clientfd,&(request.op),sizeof(request.op));

        if(request.op==OPEN){
            d_readn(clientfd,&(request.flag),sizeof(request.flag));
            d_readn(clientfd,&(request.size),sizeof(request.size));
            d_readn(clientfd,&(request.filepath),request.size); 
            if(p_flag){
                printf("\n%s",opseparator);
                print_op(request.op);
                print_flag(request.flag);
                fflush(stdout);
                printf("contenuto dimensione %d FILEPATH: %s\n",(int)strlen(request.filepath),request.filepath);
            }    
            ret=handleop(request,clientfd);
            if(p_flag)printf("ret handleconnetion %d\n",ret);
            d_writen(clientfd,&ret,sizeof(int));
            if(p_flag)printf("\n%s",opseparator);

        }
        else if(request.op==APPEND){
            d_readn(clientfd,&(request.size),sizeof(request.size));
            d_readn(clientfd,&(request.filepath),request.size);
            d_readn(clientfd,&(request.contentsize),sizeof(request.contentsize));
            d_readn(clientfd,&(request.content),request.contentsize);
            if(p_flag){
                printf("\n%s",opseparator);
                print_op(request.op);
                printf("sizepath %d\npath: %s\nsizecontent %d\ncontent: %s\n",request.size,request.filepath,request.contentsize,request.content);
            }   
            ret=handleop(request,clientfd);
            if(p_flag)printf("ret handleconnetion append %d \n",ret);
            d_writen(clientfd,&ret,sizeof(ret));                
            if(p_flag)printf("%s\n",opseparator);

        }
        else if(request.op==CLOSE){
            d_readn(clientfd,&(request.size),sizeof(request.size));
            d_readn(clientfd,&(request.filepath),request.size);
            ret=handleop(request,clientfd);
            if(p_flag)printf("CLOSEFILE RET %d\n",ret);
            d_writen(clientfd,&ret,sizeof(ret));
        }
        else if(request.op==LOCK){
            d_readn(clientfd,&(request.size),sizeof(request.size));
            d_readn(clientfd,&(request.filepath),request.size);
            ret=handleop(request,clientfd);
            d_writen(clientfd,&ret,sizeof(ret));

        }
        else if(request.op==UNLOCK){
            d_readn(clientfd,&(request.size),sizeof(request.size));
            d_readn(clientfd,&(request.filepath),request.size);
            ret=handleop(request,clientfd);
            d_writen(clientfd,&ret,sizeof(ret));
        }
        else if(request.op==READ){
            d_readn(clientfd,&(request.size),sizeof(request.size));
            d_readn(clientfd,&(request.filepath),request.size);
            ret=handleop(request,clientfd);
        }
        else if(request.op==READN){
            d_readn(clientfd,&(request.size),sizeof(request.size));
            if(p_flag)printf("request dal client %d\n",request.size);
            ret=handleop(request,clientfd);
        }
        else if(request.op==REMOVE){
            d_readn(clientfd,&(request.size),sizeof(request.size));
            d_readn(clientfd,&(request.filepath),request.size);
            ret=handleop(request,clientfd);
            d_writen(clientfd,&ret,sizeof(ret));
        }
        else if(request.op==TURNOFF){
            m_lock(&p_client_mutex);
            p_client--;
            if(p_flag)printf("handleopo pclient %d\n",p_client);
            m_unlock(&p_client_mutex);
            break;
        }
    }
   return 0;
}

/* Funzione assegnata ad ogni thread del pool */
void * threadfunction(void * arg){
    /* Fino a quando non ricevo i segnali prendo le richieste del client e me ne prendo cura */
    while(sighintquit==0 && sighup==0){
        int * fd_client=NULL;
        m_lock(&(clientsqueue->queue_mtx));//effettuo la lock sulla coda 
        if( (fd_client=(int*)(conc_queue_pop(clientsqueue)))==NULL){//se la coda è vuota 
            m_wait(&(clientsqueue->queue_cv),&(clientsqueue->queue_mtx));//aspetto una signal che mi dica che esiste almeno un elemento in coda
            if(sighintquit!=0 || sighup!=0){
                m_unlock(&(clientsqueue->queue_mtx));//faccio la unlock ed esco
                break;
            }
            fd_client=(int*)(conc_queue_pop(clientsqueue)); //e riprovo una volta piena
        }
        m_unlock(&(clientsqueue->queue_mtx));//faccio la unlock
        if(fd_client!=NULL){//se quindi esiste una connessione con un client pendente
            int fd_client_topass=*fd_client;
            free(fd_client);
            handleconnection(fd_client_topass);//mi prendo cura della richiesta
        }
    }
    return NULL;//valgrind da problemi con pthread exit perche questa chiamata intermedia  
}

/* Funzione Dispatcher per i Thread */
void* dispatcher(void * arg){  
    /* Inizializzazione */
    clientsqueue=conc_queue_create(NULL);
    
    /* Creazione del ThreadPool */
    pthread_t thread_pool[num_workers]; //creo la pool di thread 
    for (int i = 0; i < num_workers; i++){//assegno la funzione di accettazione
        pthread_create(&thread_pool[i],NULL,threadfunction,NULL);        
    }

    /* Accetto connessioni e segnalo ai workers */
    while(sighintquit==0 && sighup==0){
        fd_conn=-1;
        if(p_flag){printf("aspettando una connessione...\n");}

        /* Accetto la connessione con il client */
        fd_conn=accept(fd_socket,NULL,0);
        if(fd_conn==-1){
            if(p_flag)printf("fdconn -1\n");
            break;
        }
        if(p_flag){ printf("connessione avvenuta con %d\n",fd_conn);}
        
        /* Aggiorno il numero di client connessi contemporaneamente */
        m_lock(&p_client_mutex);
        p_client++;
        m_lock(&tolog_struct_mutex);
        if(p_client>tolog.maxconnection_tolog){
            tolog.maxconnection_tolog=p_client;
            if(p_flag)printf("tologmaxconn: %d pclient: %d\n",tolog.maxconnection_tolog,p_client);

        }
        m_unlock(&tolog_struct_mutex);
        m_unlock(&p_client_mutex);

        /* Aggiungo il client alla coda */
        int* fd_client=malloc(sizeof(int));//alloco un puntatore a intero cosi mi viene meglio a passarlo tra thread e funzioni
        *fd_client=fd_conn;//gli assegno il file descriptor del client da gestire
        m_lock(&(clientsqueue->queue_mtx));
        conc_queue_push(clientsqueue,(void*)fd_client);//e lo metto in coda 
        m_signal(&(clientsqueue->queue_cv));//segnalo che la coda ha ora almeno un elemento
        m_unlock(&(clientsqueue->queue_mtx)); 

    
    }    
    /* Ho ricevuto un Segnale,procedura di Terminazione */
    
    /* Operazione di Logging per le info della cache */
    char finalbuff[1024]={'\0'};
    final_log(finalbuff);

    /*Apro il file,Scrivo e Chiudo */
    int logfile_fd;
    if((logfile_fd=open(LOGFILE_PATH, O_WRONLY | O_APPEND | O_CREAT, 0777))==ERROR) {
            LOG_ERR(errno, "logger: opening log file");
            exit(EXIT_FAILURE);
    }
    d_writen(logfile_fd, (void*)finalbuff, strlen(finalbuff));
    if((close(logfile_fd))==ERROR) {
        LOG_ERR(errno, "logger: closing log file");
        exit(EXIT_FAILURE);
    }

    /* Segnalo al logger che sto terminando */
    m_signal(&(log_queue->queue_cv));    
    
    /* Segnalo ad ogni thread worker che il dispatcher e' pronto a joinarli */
    for (int i = 0; i < num_workers; i++){
        m_signal(&(clientsqueue->queue_cv));
    }
    /* Joining dei thread worker */
    for (int i = 0; i < num_workers; i++){
        void * exitstatus;
        pthread_join(thread_pool[i],&exitstatus);
    }

    /* Dealloco la Coda dei Client */
    if(clientsqueue){
        /* Libero la coda dei client id */
        while(conc_queue_isEmpty(clientsqueue)!=1){
            char * tmp=NULL;
            tmp=conc_queue_pop(clientsqueue);
            if(p_flag)printf("tmp %s\n",tmp);
            free(tmp);
        }
        int returndealloc=queue_dealloc_full(clientsqueue);
        if(p_flag)printf("dealloc queue dispatcher %d\n",returndealloc);
    }
    
    //pthread_exit((void*)1);
    return NULL;
}

