#include"../Headers/server_globals.h"
#include"../Headers/comm.h"
#include"../Headers/server_util.h"
#include"../Headers/DataStructures/hash_table.h"
#include"../Headers/DataStructures/conc_queue.h"

#include<pthread.h>

/*Struct*/
struct sockaddr_un{
    sa_family_t sun_family;
    char sun_path[SOCKETPATHMAX];
};



/*####################  GLOBALS DEL MAIN  #########################*/
filestats stats; //struttura per contenere i dati da inserire nel file di log
struct sockaddr_un sa; //socket



int main(int argc,char *argv[]){    

    /*#####################  INIZIALIZZAZIONE  #########################*/

    /* Setting configurazione iniziale file config*/
    initconfig(argc,argv[1]);
    tolog=tolog_init();

    /*Creo la lista dei messaggi per il thread logger*/
    log_queue=conc_queue_create(NULL);

    /*Dichiarazioni variabili e strutture*/
    int error;
    pthread_t dispatch_tid;
    pthread_t logger_tid;
    mycache=create_cache();  


    /* Gestione dei Segnali */
    signal_handling();
    
    /* Apro il file di log in scrittura */
    if( (filelog=fopen("logfile.txt","w"))==NULL ){
        perror("fopen fallita");
        exit(EXIT_FAILURE);
    }


    /*#####################  SOCKET  #########################*/
    fd_socket=socket(AF_UNIX,SOCK_STREAM,0);
    sa.sun_family=AF_UNIX;
    strncpy(sa.sun_path,socketname,SOCKETPATHMAX);
    int errore;
    if((errore=bind(fd_socket,(struct sockaddr *)&sa,sizeof(sa)))!=0){
        printf("errore:%d\n",errore);
        perror("errore bind socket");
        exit(EXIT_FAILURE);
    }
    if((listen(fd_socket,SOMAXCONN))!=0){
        perror("errore listen socket");
        exit(EXIT_FAILURE);
    }


    /*####################  CREAZIONE DEL THREAD DISPATCHER  #####################*/
    if( (error=pthread_create(&dispatch_tid,NULL, &dispatcher,&fd_socket)) !=0){//l'accettazione dei client viene effettutata da un thread separato,il dispatcher.
        errno=error;
        perror("\nErrore Creazione Dispatcher");
        exit(EXIT_FAILURE);
    }
    if( (error=pthread_create(&logger_tid,NULL, &logger_func,NULL)) !=0){
        errno=error;
        perror("\nErrore Creazione Logger");
        exit(EXIT_FAILURE);
    }

    /*###########################  CHIUSURA DEL PROGRAMMA   ##############################*/
    if( (error=pthread_join(dispatch_tid,NULL)) !=0){//JOIN  è Bloccante
        errno=error;
        perror("\nJoinando il dispatcher");
        exit(EXIT_FAILURE);
    }
    m_signal(&(log_queue->queue_cv));//segnalo al logger che ho finito 
    if( (error=pthread_join(logger_tid,NULL)) !=0){//JOIN  è Bloccante
        errno=error;
        perror("\nJoinando il logger ");
        exit(EXIT_FAILURE);
    }
    /*##############################  PULIZIA DELLA MEMORIA E UNLINK ############################*/
    if(close(fd_socket)==ERROR){
        printf("errno: %d",errno);
        perror("close fallita\n");
        exit(EXIT_FAILURE);
    }
    unlink(socketname);    
    fclose(filelog);
    destroy_cache(mycache);
    queue_dealloc_full(log_queue);
    printf("SERVER HA CONCLUSO\n");
    return 0;
}


