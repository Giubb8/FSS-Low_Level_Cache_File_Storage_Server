#pragma once

#include<stdio.h>
#include<stdarg.h>
#include<sys/types.h>
#include<sys/socket.h>
#include "../Headers/server_globals.h"
#include "../Headers/server_util.h"
#include "../Headers/conc_queue.h"
#include "../Headers/cache.h"

#define MAXPATH 100 //lunghezza del path massimo per raggiungere la socket

typedef struct message{
  int op;
  int flag;
  int more;
  int size;
  char filepath[MAXPATH];
}msg;

void * dispatcher(void * arg);
void * threadfunction(void * arg);
void * handleconnection(void * arg);
int handleop(msg request,int clientfd);


int m_lock(pthread_mutex_t* mtx);
int m_unlock(pthread_mutex_t* mtx);
int m_wait(pthread_cond_t* cond, pthread_mutex_t* mtx);
int m_signal(pthread_cond_t* cond);

int handle_o(char * pathname,int flags);
int handle_r(char *pathname);
