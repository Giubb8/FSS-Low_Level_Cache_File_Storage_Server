
#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<pthread.h>
#include<string.h>
#include<unistd.h>
#include<stdint.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include "../Headers/comm.h"
#include <fcntl.h>


/*####################  STRUTTURE E GLOBALS  ############################*/

/* Struttura per memorizzare i messaggi ricevuti dal client*/
conc_queue * clientsqueue=NULL; //lista dei clienti che devono essere serviti



/*####################  FUNZIONI DI UTILITY PER LE COMUNICAZIONI ############################*/
 

/* Lock con gestione errore*/
int m_lock(pthread_mutex_t* mtx){
  int error;
  if( (error=pthread_mutex_lock(mtx)) !=0) {
    errno=error;
    perror("\nLocking mutex: ");
    exit(EXIT_FAILURE);
  }
  if(comm_flag){printf("Mutex locked\n");}
  return 1;
}

/* Unlock con gestione errore*/
int m_unlock(pthread_mutex_t* mtx) {
  int error;
  if( (error=pthread_mutex_unlock(mtx))!=0) {
    errno=error;
    perror("\nUnlocking mutex: ");
    _exit(EXIT_FAILURE);
  }
  if(comm_flag){printf("Mutex unlocked\n");}
  return 1;
}

/* Wait con gestione errore */
int m_wait(pthread_cond_t* cond, pthread_mutex_t* mtx) {
  int error;
  if((error=pthread_cond_wait(cond, mtx))!=0) {
    errno=error;
    perror("\nPreparing to wait for cond: ");
    _exit(EXIT_FAILURE);
  }
  if(comm_flag){printf("Wait in corso\n");}

  return 1;
}

/* Signal con gestione errore */
int m_signal(pthread_cond_t* cond){
  int error;
  if((error=pthread_cond_signal(cond))!=0) {
    errno=error;
    perror("\nSignaling cond: ");
    _exit(EXIT_FAILURE);
  }
  if(comm_flag){printf("Signal inviata\n");}
  return 1;
}



/*####################  FUNZIONI PER LE COMUNICAZIONI E GESTIONE RICHIESTE  ############################*/
int handleop(msg request,int clientfd){
    if(request.op==OPEN){
      int lock=0;
      int create=0;
      if(request.flag==O_BOTH){
          lock=1;
          create=1;
      }
      else if(request.flag==O_CREATE){
          create=1;
      }
      else if(request.flag==O_LOCK){
          lock=1;
      }
      else{
        perror("Flag non riconosciuto\n");
        exit(EXIT_FAILURE);
      }

      int ret=OpenCachedFile(mycache,request.filepath,clientfd,create,lock);
      fflush(stdout);
      printf("ret opencachedfile: %d\n",ret);
      return ret;
    }
    if(request.op==APPEND){
      int ret=AppendTo(mycache,request.filepath,clientfd,request.content);
      fflush(stdout);
      return ret;
    }
    if(request.op==CLOSE){
        int ret=CloseFile(mycache,request.filepath,request.size,clientfd);
        fflush(stdout);
        return ret;
    }
    if(request.op==LOCK){
        int ret=LockFile(mycache,request.filepath,clientfd);
        fflush(stdout);
        return ret;
    }
    if(request.op==UNLOCK){
        int ret=UnlockFile(mycache,request.filepath,clientfd);
        fflush(stdout);
        return ret;
    }
    if(request.op==READ){
        int ret=ReadFile(mycache,request.filepath,clientfd);
        fflush(stdout);
        return ret;
    }
    if(request.op==READN){
        int ret=ReadNFile(mycache,request.size,clientfd);
        fflush(stdout);
        return ret;
    }
    if(request.op==REMOVE){
        int ret=RemoveFile(mycache,request.filepath,clientfd);
        fflush(stdout);
        return ret;
    }

}


void * handleconnection(void * arg){
    //TODO forse free(arg)?
    int ret;
    int clientfd=(int)(uintptr_t)arg;
    while (/*sighup==0 &&*/ sighintquit==0){   //se ricevo sighint o sigquit non devo più gestire le richieste con i client connessi  
        msg request={0};

        readn(clientfd,&(request.op),sizeof(request.op));

        if(request.op==OPEN){
            readn(clientfd,&(request.flag),sizeof(request.flag));
            readn(clientfd,&(request.size),sizeof(request.size));
            readn(clientfd,&(request.filepath),request.size);
            if(p_flag){
                printf("\n%s",opseparator);
                print_op(request.op);
                print_flag(request.flag);
                printf("contenuto: %s\n",request.filepath);
            }    
            ret=handleop(request,clientfd);
            size_t ret_t=ret;
            printf("ret handleconnetion %d ret_t %ld\n",ret,ret_t);
            int f=writen(clientfd,&ret,sizeof(int));
            //int f1=writen(clientfd,&ret,sizeof(int));
            //printf("f %d f1 %d\n",f,f1);
            printf("\n%s",opseparator);

        }
        else if(request.op==APPEND){
            readn(clientfd,&(request.size),sizeof(request.size));
            readn(clientfd,&(request.filepath),request.size);
            readn(clientfd,&(request.contentsize),sizeof(request.contentsize));
            readn(clientfd,&(request.content),request.contentsize);
            if(p_flag){
                printf("\n%s",opseparator);
                print_op(request.op);
                printf("sizepath %d\npath: %s\nsizecontent %d\ncontent: %s\n",request.size,request.filepath,request.contentsize,request.content);
            }   
            ret=handleop(request,clientfd);
            writen(clientfd,&ret,sizeof(ret));                
            printf("%s\n",opseparator);

        }
        else if(request.op==CLOSE){
            readn(clientfd,&(request.size),sizeof(request.size));
            readn(clientfd,&(request.filepath),request.size);
            ret=handleop(request,clientfd);
            writen(clientfd,&ret,sizeof(ret));
        }
        else if(request.op==LOCK){
            readn(clientfd,&(request.size),sizeof(request.size));
            readn(clientfd,&(request.filepath),request.size);
            ret=handleop(request,clientfd);
            writen(clientfd,&ret,sizeof(ret));

        }
        else if(request.op==UNLOCK){
            readn(clientfd,&(request.size),sizeof(request.size));
            readn(clientfd,&(request.filepath),request.size);
            ret=handleop(request,clientfd);
            writen(clientfd,&ret,sizeof(ret));
        }
        else if(request.op==READ){
            readn(clientfd,&(request.size),sizeof(request.size));
            readn(clientfd,&(request.filepath),request.size);
            ret=handleop(request,clientfd);
        }
        else if(request.op==READN){
            readn(clientfd,&(request.size),sizeof(request.size));
            if(request.size==0){
                writen(clientfd,&(mycache->num_files),sizeof(int));
            }
            ret=handleop(request,clientfd);
        }
        else if(request.op==REMOVE){
            readn(clientfd,&(request.size),sizeof(request.size));
            readn(clientfd,&(request.filepath),request.size);
            ret=handleop(request,clientfd);
            writen(clientfd,&ret,sizeof(ret));
        }
        else if(request.op==TURNOFF){
            m_lock(&p_client_mutex);
            p_client--;
            printf("handleopo pclient %d\n",p_client);
            m_unlock(&p_client_mutex);
            break;
        }
    }
   return 0;
}

/* Funzione assegnata ad ogni thread del pool */
void * threadfunction(void * arg){
    //fino a quando non ricevo un segnale lavoro
    //printf("threadfunction, tid %d\n",(int)syscall(__NR_gettid));
    while(sighintquit==0 && sighup==0){
        int * fd_client;
        m_lock(&(clientsqueue->queue_mtx));//effettuo la lock sulla coda 
        if( (fd_client=(conc_queue_pop(clientsqueue)))==NULL){//se la coda è vuota 
            m_wait(&(clientsqueue->queue_cv),&(clientsqueue->queue_mtx));//aspetto una signal che mi dica che esiste almeno un elemento in coda
            fd_client=(conc_queue_pop(clientsqueue)); //e riprovo una volta piena
        }
        m_unlock(&(clientsqueue->queue_mtx));//faccio la unlock
        if(fd_client!=NULL){//se quindi esiste una connessione con un client pendente
            handleconnection(fd_client);//mi prendo cura della richiesta
        }
    }
    printf("uscito da threadfun sighint==%d\n",sighintquit);
    pthread_exit((void*)1);

}


void* dispatcher(void * arg){  
    /*Inizializzazione */

    clientsqueue=conc_queue_create(NULL);
    
    /* ###### Creazione del ThreadPool  ######### */
    pthread_t thread_pool[num_workers]; //creo la pool di thread 
    for (int i = 0; i < num_workers; i++){//assegno la funzione di accettazione
        pthread_create(&thread_pool[i],NULL,threadfunction,NULL);
        pthread_detach(thread_pool[i]);
    }

    /* ########## Accetto connessioni e segnalo ai workers ############*/
    while(sighintquit==0 && sighup==0){
        fd_conn=-1;
        if(p_flag){printf("aspettando una connessione...\n");}
        fd_conn=accept(fd_socket,NULL,0);
        if(fd_conn==-1){
            printf("fdconn -1\n");
            break;
        }
        if(p_flag){ printf("connessione avvenuta con %d\n",fd_conn);}
        m_lock(&p_client_mutex);
        p_client++;

        m_lock(&tolog_struct_mutex);
        if(p_client>tolog.maxconnection_tolog){
            tolog.maxconnection_tolog=p_client;
            printf("tologmaxconn: %d pclient: %d\n",tolog.maxconnection_tolog,p_client);

        }
        m_unlock(&tolog_struct_mutex);

        m_unlock(&p_client_mutex);
        int fd_client;//alloco un puntatore a intero cosi mi viene meglio a passarlo tra thread e funzioni
        fd_client=fd_conn;//gli assegno il file descriptor del client da gestire
  
        m_lock(&(clientsqueue->queue_mtx));
    
        conc_queue_push(clientsqueue,(void*)(uintptr_t)fd_client);//e lo metto in coda 
        queue_print(clientsqueue);
        m_signal(&(clientsqueue->queue_cv));//segnalando che la coda ha ora almeno un elemento
    
        m_unlock(&(clientsqueue->queue_mtx)); 
    
    }    
    /* Operazione di Logging per le info della cache */
    char finalbuff[1024];
    final_log(finalbuff);
    /*Apro il file,Scrivo e Chiudo */
    int logfile_fd;
    if((logfile_fd=open(LOGFILE_PATH, O_WRONLY | O_APPEND | O_CREAT, 0777))==ERROR) {
            LOG_ERR(errno, "logger: opening log file");
            exit(EXIT_FAILURE);
        }

    writen(logfile_fd, (void*)finalbuff, strlen(finalbuff));
    if((close(logfile_fd))==ERROR) {
        LOG_ERR(errno, "logger: closing log file");
        exit(EXIT_FAILURE);
    }
    m_signal(&(log_queue->queue_cv));
    
    pthread_exit((void*)1);

}

