#include"../Headers/server_util.h"
#include<pthread.h>

void* handle_conn(void * fd_conn);
void spawnthread(int fd_conn);
int main(int argc,char *argv[]){
    
    /*setting configurazione iniziale file config*/
    initconfig(argc,CONFIG_PATH);

    /*Inizializzazione Socket*/
    int fd_socket,fd_conn;
    struct sockaddr_un sa;
    strcpy(socketname,argv[1]);//TODO in teoria prende il nome della socket dal file config.txt non da qua,ridondante con la funzione di sopra cambiare
    strcpy(sa.sun_path,socketname);
    sa.sun_family=AF_UNIX;

    fd_socket=socket(AF_UNIX,SOCK_STREAM,0);
    bind(fd_socket,(struct sockaddr *)&sa,sizeof(sa));
    listen(fd_socket,SOMAXCONN);

    while(TRUE){
        fd_conn=accept(fd_socket,NULL,0);    
        printf("connessione accettata con fd:%d\n",fd_conn);
        spawnthread(fd_conn);     
    }
    return 0;
}



void* handle_conn(void * fd_conn){
    int i=0;    
    int fd=(int)fd_conn;
    int err;
    do{
        char str[55];
        err=read(fd,str,sizeof(str));
        printf("err: %d\n",err);
        //char * buffer=malloc(sizeof(str) * sizeof(char));
        printf("sono server2 pre leggo: %s\n",str);
        fflush(stdout);
        memset(str,0,sizeof(str));

    }while(err>0);
    pthread_exit(1);   
}

void spawnthread(int fd_conn){
    pthread_t tid;
    int err;
    if((err=pthread_create(&tid,NULL,&handle_conn,(void *)fd_conn))!=0){
            perror("errore creazione thread");
        }
    if((err=pthread_detach(tid))!=0){
        perror("detaching");
    }
    return;
}
