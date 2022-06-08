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

/*Const*/
#define _GNU_SOURCE
#define TRUE 1
#define MAXSOCKETNAME 100
#define MAXARGUMENTLENGHT 256
#define WAIT_CONN_TRY 400 // msec attesa tra un tentativo di connessione e l'altro
#define TIMEOUT 10 //secondi prima di timeout connection
#define SOCKETPATHMAX 100 //lunghezza del path massimo per raggiungere la socket
#define CONFIG_PATH "./config.txt"

/*Struct*/
struct sockaddr_un{
    sa_family_t sun_family;
    char sun_path[SOCKETPATHMAX];
};

/*Flags & Globals*/
char socketname[MAXSOCKETNAME];
int num_workers=0;
int memory_dimension=0;
int num_max_file=0;//numero massimo di file memorizzabili
int wait_time=0;//tempo di attesa tra richieste successive,associato a -t
int fd_socket=-1;//file descriptor della socket
char * separator="~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";
struct sockaddr_un sa; //socket
filestats stats; 

/*Prototipi*/

void initconfig(int argc,char const * pos);
/*Funzioni*/


/*parsing configurazione iniziale da config.txt*/
void initconfig(int argc,char const * pos){

    /*controlli*/
   /* if(argc!=2){
        printf("input file mancante\n");
        exit(EXIT_FAILURE);
    }
    if(strcmp("config.txt",pos)!=0){
        printf("file config.txt non trovato\n");
        exit(EXIT_FAILURE);
    }*/

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
      if(strlen(Csockname)<MAXSOCKETNAME){
          strcpy(socketname,Csockname);  
    }
    printf("numero di workers: %d numero max file: %d dimensione memoria %d socketname: %s\n",num_workers,num_max_file,memory_dimension,socketname);
    fclose(fd);
}