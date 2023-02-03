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
#include"server_globals.h"
#include"./DataStructures/cache.h"




/*################################ COSTANTI E FLAG  ############################################*/
#define _GNU_SOURCE
#define TRUE 1
#define MAXSOCKETNAME 100
#define MAXARGUMENTLENGHT 256
#define WAIT_CONN_TRY 400 // msec attesa tra un tentativo di connessione e l'altro
#define TIMEOUT 10 //secondi prima di timeout connection
#define SOCKETPATHMAX 100 //lunghezza del path massimo per raggiungere la socket
#define O_LOCK 125 //flag o_lock
#define O_CREATE 126 //flag o_create
#define O_BOTH 127 //flag per selezionare entrambi i flag
#define NO_FLAG 128 //flag nullo

#define CONFIG_PATH "../Config_files/config.txt"
#define LOGFILE_PATH "../Source/logfile.txt"

/*################################ PROTOTIPI ############################################*/

void initconfig(int argc,char const * pos);
void sig_intquit_handler();
void sig_sighup_handler();
void signal_handling();
int readn(int source, void* buf, int toread);
int writen(int source, void* buf, int towrite);
int d_readn(int source, void* buf, int toread);
int d_writen(int source, void* buf, int towrite);
void print_op(int opcode);
void print_flag(int flag);
void * logger_func(void* arg);
void final_log(char * buff);

/*################################ FUNZIONI  ############################################*/


// Funzione per Gestione Errori per il log 
#define LOG_ERR(err_code, err_desc) \
  errno=err_code; \
  fprintf(stderr, "%s: %s\n", err_desc, strerror(err_code));

/* Funzione per il logging,vaargs in maiuscolo è per fare riferirsi a va args come macro,define perche dava comportamenti strani definita normalmente per la variadicita*/ 
#define log_op(fmt, ...) \
    { \
        int temperr;\
        char* buffer=malloc(1024*sizeof(char)); \
        if(!buffer) {LOG_ERR(temperr, "logging: allocazione buffer"); return ERROR;} \
        memset(buffer, '\0', 1024); \
        sprintf(buffer, fmt, ##__VA_ARGS__);\
        m_lock(&(log_queue->queue_mtx));\
        if((temperr=conc_queue_push(log_queue, (void*)buffer))==ERROR) { \
            LOG_ERR(errno, "logging: push sulla queue"); return ERROR; \
        } \
        m_signal(&(log_queue->queue_cv));\
        m_unlock(&(log_queue->queue_mtx));\
    }











