#pragma once

#include<stdio.h>
#include<stdarg.h>
#include<sys/types.h>
#include<sys/socket.h>

void * dispatcher(void * arg);
void * threadfunction(void * arg);
void * handleconnection(void * arg);

void m_lock(pthread_mutex_t* mtx);
void m_unlock(pthread_mutex_t* mtx);
void m_wait(pthread_cond_t* cond, pthread_mutex_t* mtx);
void m_signal(pthread_cond_t* cond);
