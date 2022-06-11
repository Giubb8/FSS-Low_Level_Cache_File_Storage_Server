#pragma once

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<ctype.h>
#include<unistd.h>
#include<time.h>
#include<errno.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<sys/socket.h>
#include"cache.h"
#include"server_globals.h"


/*################################ COSTANTI ############################################*/
#define _GNU_SOURCE
#define TRUE 1
#define MAXSOCKETNAME 100
#define MAXARGUMENTLENGHT 256
#define WAIT_CONN_TRY 400 // msec attesa tra un tentativo di connessione e l'altro
#define TIMEOUT 10 //secondi prima di timeout connection
#define SOCKETPATHMAX 100 //lunghezza del path massimo per raggiungere la socket
#define CONFIG_PATH "./config.txt"


/*################################ PROTOTIPI ############################################*/

void initconfig(int argc,char const * pos);
void sig_intquit_handler();
void sig_sighup_handler();
void signal_handling();



/*################################ FUNZIONI ############################################*/


/* parsing configurazione iniziale da config.txt  */
void initconfig(int argc,char const * pos){

    /*variabili temporanee*/
    int Cworkers;
    int Cnumfile;
    char Csockname[MAXSOCKETNAME]={0};  
    int Cmemorydimension; 


    /*apertura file in lettura*/
    FILE*fd;
    if((fd=fopen(pos,"r"))==NULL){
        perror("aprendo il file");
        fclose(fd);
        exit(EXIT_FAILURE);
    }

    /*leggo i valori*///[^;]==tutti i valori eccetto ;
    fscanf(fd,"%d;%d;%d;%[^;]",&Cworkers,&Cnumfile,&Cmemorydimension,Csockname);
    num_workers=Cworkers;
    num_max_file=Cnumfile;
    memory_dimension=Cmemorydimension;
    if(num_workers==0 || num_max_file==0 || memory_dimension==0){
        perror("file config non puo prevedere un parametro a 0");
        exit(EXIT_FAILURE);    
    }
    if(strlen(Csockname)<MAXSOCKETNAME){
          strcpy(socketname,Csockname);  
    }
    else{
        perror("file config sockentame troppo lungo");
        exit(EXIT_FAILURE);  
    }
    if(p_flag)printf("numero di workers: %d numero max file: %d dimensione memoria %d socketname: %s\n",num_workers,num_max_file,memory_dimension,socketname);
    fclose(fd);
}


/* #################### HANDLING SEGNALI ############################ */

//handling sigint e sigquit
void sig_intquit_handler(){
    
    printf("ciaon\n");
    fprintf(filelog,"%s %d",socketname,fd_conn);
    exit(EXIT_FAILURE);
}

//handling sighup
void sig_sighup_handler(){
    printf("ciaon\n");
    exit(EXIT_FAILURE);
}

//assegnamento comportamento per i segnali sigint,sigquit,sighup
void signal_handling(){
    struct sigaction s_iq;
    struct sigaction s_h;
    memset(&s_iq,0,sizeof(s_iq));
    memset(&s_iq,0,sizeof(s_h));
    s_iq.sa_handler=sig_intquit_handler;
    s_h.sa_handler=sig_sighup_handler;
    sigaction(SIGINT,&s_iq,NULL);
    sigaction(SIGQUIT,&s_iq,NULL);
    sigaction(SIGHUP,&s_h,NULL);   
}

