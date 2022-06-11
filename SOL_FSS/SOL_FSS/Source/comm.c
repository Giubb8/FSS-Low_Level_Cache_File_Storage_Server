#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<pthread.h>
#include<string.h>
#include<unistd.h>
#include "../Headers/comm.h"
#include"../Headers/server_globals.h"
#include "../Headers/conc_queue.h"

#define MAXPATH 100 //lunghezza del path massimo per raggiungere la socket

conc_queue * clientsqueue=NULL; //lista dei clienti che devono essere serviti
void *  threadfunction(){
  printf("ciao sono thread function\n");
}


void* dispatcher(){ //TODO  da testare
    
    /* Creazione del threadpool */
    pthread_t thread_pool[num_workers]; //creo la pool di thread 
    for (int i = 0; i < num_workers; i++){//assegno la funzione di accettazione
        pthread_create(&thread_pool[i],NULL,threadfunction,NULL);//TODO FORSE DEVO INIZIALIZZARLI ?
    }
    clientsqueue=conc_queue_create(NULL);


    /* Accetto le connessioni e lo segnalo ai thread worker*/
    while(sighintquit==0 && sighup==0){
    if(p_flag){printf("aspettando una connessione...\n");}
    fd_conn=accept(fd_socket,NULL,0);
    if(p_flag){ printf("connessione avvenuta con %d\n",fd_conn);}
    int fd_client;//alloco un puntatore a intero cosi mi viene meglio a passarlo tra thread e funzioni
    fd_client=fd_conn;//gli assegno il file descriptor del client da gestire
  
    pthread_mutex_lock(&mutexqueue);
    conc_queue_push(clientsqueue,fd_client);//e lo metto in coda 
    queue_print(clientsqueue);
    //pthread_cond_signal(&condvqueue);//segnalando che la coda ha ora almeno un elemento
    pthread_mutex_unlock(&mutexqueue); 
    
}
    pthread_exit(1);

}

