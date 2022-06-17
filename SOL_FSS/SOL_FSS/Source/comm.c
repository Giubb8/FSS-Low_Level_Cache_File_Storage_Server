
#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<pthread.h>
#include<string.h>
#include<unistd.h>
#include<stdint.h>
#include "../Headers/comm.h"


/*####################  STRUTTURE E GLOBALS  ############################*/

/* Struttura per memorizzare i messaggi ricevuti dal client*/




conc_queue * clientsqueue=NULL; //lista dei clienti che devono essere serviti



/*####################  FUNZIONI DI UTILITY PER LE COMUNICAZIONI ############################*/
 


/* Lock con gestione errore*/
int m_lock(pthread_mutex_t* mtx) {
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
int m_signal(pthread_cond_t* cond) {
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
int handleop(msg request,int clientfd){
    if(request.op==OPEN){
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
      }//TODO forse non casi completi
      int ret=OpenCachedFile(mycache,request.filepath,clientfd,create,lock);
      if(ret==SUCCESS){
              return SUCCESS;
      }
      
    }
}


void * handleconnection(void * arg){
    //TODO forse free(arg)?
    int ret;
    int clientfd=(int)(uintptr_t)arg;
    msg request;
    while (sighup==0 && sighintquit==0){
        readn(clientfd,&(request.op),sizeof(request.op));
        readn(clientfd,&(request.flag),sizeof(request.flag));
        readn(clientfd,&(request.size),sizeof(request.size));
        readn(clientfd,&(request.filepath),request.size);


        if(p_flag)printf("ricevuta operazione %d\nflag %d\ndimensione %d\ncontenuto %s\n",request.op,request.flag,request.size,request.filepath);
        ret=handleop(request,clientfd);
        writen(clientfd,&ret,sizeof(ret));//TODO VEDERE COME VA OPERAZIONE E SCRIVERE RISULTATO AL CLIENT CHE HA EFFETTUATO RICHIESTA
    }
   return 0;
}

/* Funzione assegnata ad ogni thread del pool */
void * threadfunction(void * arg){
    //fino a quando non ricevo un segnale lavoro
    while(sighintquit==0 && sighup==0){
        int * fd_client;
        m_lock(&(clientsqueue->queue_mtx));//effettuo la lock sulla coda 
        if( (fd_client=(conc_queue_pop(clientsqueue)))==NULL){//se la coda è vuota 
            m_wait(&(clientsqueue->queue_cv),&(clientsqueue->queue_mtx));//aspetto una signal che mi dica che esiste almeno un elemento in coda
            fd_client=(conc_queue_pop(clientsqueue)); //e riprovo una volta piena
        }
        m_unlock(&(clientsqueue->queue_mtx));//faccio la unlock
        if(fd_client!=NULL){//se quindi esiste una connessione con un client pendente
            handleconnection(fd_client);//mi prendo cura della richiesta
        }
    }
    pthread_exit((void*)1);

}


void* dispatcher(void * arg){  
    /*Inizializzazione */

    clientsqueue=conc_queue_create(NULL);
    
    /* ###### Creazione del ThreadPool  ######### */
    pthread_t thread_pool[num_workers]; //creo la pool di thread 
    for (int i = 0; i < num_workers; i++){//assegno la funzione di accettazione
        pthread_create(&thread_pool[i],NULL,threadfunction,NULL);//TODO FORSE DEVO INIZIALIZZARLI ?

    }

    /* ########## Accetto connessioni e segnalo ai workers ############*/
    while(sighintquit==0 && sighup==0){
    
        if(p_flag){printf("aspettando una connessione...\n");}
        fd_conn=accept(fd_socket,NULL,0);
        if(p_flag){ printf("connessione avvenuta con %d\n",fd_conn);}
    
        int fd_client;//alloco un puntatore a intero cosi mi viene meglio a passarlo tra thread e funzioni
        fd_client=fd_conn;//gli assegno il file descriptor del client da gestire
  
        m_lock(&(clientsqueue->queue_mtx));
    
        conc_queue_push(clientsqueue,(void*)(uintptr_t)fd_client);//e lo metto in coda 
        queue_print(clientsqueue);
        m_signal(&(clientsqueue->queue_cv));//segnalando che la coda ha ora almeno un elemento
    
        m_unlock(&(clientsqueue->queue_mtx)); 
    
    }
    pthread_exit((void*)1);

}

