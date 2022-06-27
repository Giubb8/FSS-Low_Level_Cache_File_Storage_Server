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
    initconfig(argc,CONFIG_PATH);

    /*Dichiarazioni variabili e strutture*/
    int error;
    pthread_t dispatch_tid;
    mycache=create_cache();  

    /* Setting configurazione iniziale file config*/

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

    if((bind(fd_socket,(struct sockaddr *)&sa,sizeof(sa)))!=0){
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
        perror("\nCreating dispatcher thread: ");
        exit(EXIT_FAILURE);
    }

    /*###########################  CHIUSURA DEL PROGRAMMA   ##############################*/
    if( (error=pthread_join(dispatch_tid,NULL)) !=0){//JOIN  è Bloccante
        errno=error;
        perror("\nJoining dispatcher thread: ");
        exit(EXIT_FAILURE);
    }

    /*##############################  PULIZIA DELLA MEMORIA E UNLINK ############################*/
    close(fd_socket);
    unlink(socketname);    
    fclose(filelog);
    exit(EXIT_SUCCESS);
    return 0;
}


