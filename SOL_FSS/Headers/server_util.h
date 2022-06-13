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
