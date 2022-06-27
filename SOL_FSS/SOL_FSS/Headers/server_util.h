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




/*################################ COSTANTI ############################################*/
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
/*################################ PROTOTIPI ############################################*/

void initconfig(int argc,char const * pos);
void sig_intquit_handler();
void sig_sighup_handler();
void signal_handling();
int readn(long fd, void *buf, size_t size);
int writen(long fd, void *buf, size_t size);
void print_op(int opcode);
void print_flag(int flag);
