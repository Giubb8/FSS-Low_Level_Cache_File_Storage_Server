
#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<pthread.h>
#include<string.h>
#include<unistd.h>
#include "../Headers/server_util.h"
#include "../Headers/comm.h"
#include"../Headers/server_globals.h"
#include "../Headers/conc_queue.h"

#define MAXPATH 100 //lunghezza del path massimo per raggiungere la socket

conc_queue * clientsqueue=NULL; //lista dei clienti che devono essere serviti

 
void m_lock(pthread_mutex_t* mtx) {

  int error;
  if( (error=pthread_mutex_lock(mtx)) !=0) {
    perror("\nLocking mutex: ");
    exit(EXIT_FAILURE);
  }
  if(p_flag){printf("Mutex locked\n");}
  return;
}

void m_unlock(pthread_mutex_t* mtx) {
  int error;
  if( (error=pthread_mutex_unlock(mtx))!=0) {
    errno=error;
    perror("\nUnlocking mutex: ");
    _exit(EXIT_FAILURE);
  }
  if(p_flag){printf("Mutex unlocked\n");}
  return;
}

void m_wait(pthread_cond_t* cond, pthread_mutex_t* mtx) {
  int error;
  if((error=pthread_cond_wait(cond, mtx))!=0) {
    errno=error;
    perror("\nPreparing to wait for cond: ");
    _exit(EXIT_FAILURE);
  }
  if(p_flag){printf("Wait in corso\n");}

  return;
}

void m_signal(pthread_cond_t* cond) {
  int error;
  if((error=pthread_cond_signal(cond))!=0) {
    errno=error;
    perror("\nSignaling cond: ");
    _exit(EXIT_FAILURE);
  }
  if(p_flag){printf("Signal inviata\n");}
  return;
}

void * handleconnection(void * arg){
    printf("ciao\n");
}


void * threadfunction(void * arg){
  

    while(sighintquit==0 && sighup==0){//TODO vedere quando termina

        int * fd_client;
        m_lock(&(clientsqueue->queue_mtx));//effettuo la lock sulla coda //TODO NON SO SE LA LOCK VA VEBE NOSI &
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
    /* ###### Creazione del ThreadPool  ######### */
    clientsqueue=conc_queue_create(NULL);

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
    
        conc_queue_push(clientsqueue,(void*)fd_client);//e lo metto in coda 
        queue_print(clientsqueue);
        m_signal(&(clientsqueue->queue_cv));//segnalando che la coda ha ora almeno un elemento
    
        m_unlock(&(clientsqueue->queue_mtx)); 
    
    }
    pthread_exit((void*)1);

}

