#include"../Headers/server_util.h"
#include <fcntl.h>


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
        perror("aprendo il file fd");
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
    
    if(p_flag)printf("Handling segnale SIGHINTQUIT\n");
    sighintquit=1;
    shutdown(fd_socket,SHUT_RD);
    return;
}

//handling sighup
void sig_sighup_handler(){
    if(p_flag)printf("Handling segnale SIGHUP\n");
    sighup=1;
    shutdown(fd_socket,SHUT_RD);
    return;
}

//assegnamento comportamento per i segnali sigint,sigquit,sighup
void signal_handling(){
    struct sigaction s_iq;
    struct sigaction s_h;
    memset(&s_iq,0,sizeof(s_iq));
    memset(&s_h,0,sizeof(s_h));
    s_h.sa_flags=0;//necessario per errore valgrind,memset non bastava
    //s_iq.sa_flags=0;
    s_iq.sa_handler=sig_intquit_handler;
    s_h.sa_handler=sig_sighup_handler;
    sigaction(SIGINT,&s_iq,NULL);
    sigaction(SIGQUIT,&s_iq,NULL);
    sigaction(SIGHUP,&s_h,NULL);   
}

// Wrapper function of the read SysCall handling the interrupted-reading problem
int readn(int source, void* buf, int toread) {

  int bleft;     // Bytes left to read
  int bread;     // Bytes read until now
  bleft=toread;     // Before the start of the stream, nothing has been read
  while(bleft>0) {
    if((bread=read(source, buf, bleft)) < 0) {     // If an error verified
      if(bleft==toread) return -1;     // If nothing has been read, return the error state
      else break;     // If the error happened during the stream of data, return the number of bytes read
    }
    else if(bread==0) break;  // read operation completed
    bleft-=bread;   // Updates the number of bytes left (subtracting those just read)
    buf=(char*)buf+bread;   // Updates the current position of buffer pointer
  }
  return (toread-bleft);    // Returns the total number of bytes read
}

// Wrapper function of the write SysCall handling the interrupted-writing problem
int writen(int source, void* buf, int towrite) {
  int bleft;    // Bytes left to write
  int bwritten;   // Bytes written until now
  bleft=towrite;   // Before the start of the stream, nothing has been written
  while(bleft>0) {
    if((bwritten=write(source, buf, bleft)) < 0) {    // If an interruption verified
      if(bleft==towrite) return -1;    // If nothing has been written, return the error state
      else break;   // If the interruption happened during the stream of data, return the number of bytes written
    }
    else if (bwritten==0) break;  // write operation completed
    bleft-=bwritten;    // Updates the number of bytes left (subtracting those just written)
    buf=(char*)buf+bwritten;    // Updates the current position of buffer pointer
  }
  return (towrite-bleft);   // Returns the total number of bytes written
}

/* Funzione per effettuare le print dei nomi delle operazioni */
void print_op(int opcode){
    static int n_op=1;
    if(p_flag)printf("%d ",n_op);
    switch (opcode){
    case 0:
        if(p_flag)printf("SERVER TURNOFF\n");
        break;
    case 1:
        if(p_flag)printf("SERVER OPEN\n");
        break;
    case 2:
        if(p_flag)printf("SERVER READ\n");
        break;
    case 3:
        if(p_flag)printf("SERVER READN\n");
        break;
    case 4:
        if(p_flag)printf("SERVER WRITE\n");
        break;
    case 5:
        if(p_flag)printf("SERVER REMOVE\n");
        break;
    case 6:
        if(p_flag)printf("SERVER CLOSE\n");
        break;
    case 7:
        if(p_flag)printf("SERVER APPEND\n");
        break;
    case 8:
        if(p_flag)printf("SERVER LOCK\n");
        break;
    case 9:
        if(p_flag)printf("SERVER UNLOCK\n");
        break;
    default:
        break;
    }
    n_op++;
    
    return;
    
}

/* Funzione per Stampare i flag delle funzioni */
void print_flag(int flag){
    switch (flag){
    case O_BOTH:
        if(p_flag)printf("FLAG OBOTH\n");
        break;
    case NO_FLAG:
        if(p_flag)printf("FLAG NOFLAG\n");
        break;
    case O_CREATE:
        if(p_flag)printf("FLAG OCREATE\n");
        break;
    case O_LOCK:
        if(p_flag)printf("FLAG OLOCK\n");
        break;

    default:
        break;
    }
}

/* Funzione per effettuare il logging sul file apposito */
void* logger_func(void* arg) {
    int temperr;
    int logfile_fd;
    char* buffer=NULL;
    fflush(stdout);
    /* Fino a quando non ricevo i segnali eseguo...*/
    while(sighintquit==0 && sighup==0) {
        fflush(stdout);

        //locko la queue 
        m_lock(&(log_queue->queue_mtx));
        //controllo se sono stati inseriti nuovi elementi
        while(!(buffer=(char*)conc_queue_pop(log_queue)) && (sighintquit==0) && (sighup==0)) {
            if(p_flag)printf("prima della wait logger func,sighup: %d\n",sighup);
            m_wait(&(log_queue->queue_cv), &(log_queue->queue_mtx));
        }
        fflush(stdout);

        m_unlock(&(log_queue->queue_mtx));

        /* Ricontrollo i Segnali */
        if(sighintquit==1 || sighup==1){
            if(p_flag)printf("logger: func break\n");
            break;   // server termina
        }
            
        /* Apro il file*/
        if((logfile_fd=open(LOGFILE_PATH, O_WRONLY | O_APPEND | O_CREAT, 0777))==ERROR) {
            LOG_ERR(errno, "logger: aprendo il file");
            exit(EXIT_FAILURE);
        }
        /* Scrivo sul file */
        writen(logfile_fd, (void*)buffer, strlen(buffer));

        /* E lo chiudo*/
        if((close(logfile_fd))==ERROR) {
            LOG_ERR(errno, "logger: chiudendo il file ");
            exit(EXIT_FAILURE);
        }

        if(buffer) {free(buffer); buffer=NULL;}
    }
    if(p_flag)printf("uscito da logger function sighint==%d\n",sighintquit);
    fflush(stdout);
    fflush(stderr);
    
    if(buffer)
        free(buffer);
    //pthread_exit((void*)1);
    return NULL;
}

/* Funzione per preparare il log delle statistiche relative alla cache */
void final_log(char * buff){
    /*Inizializzazione delle stringhe da copiare*/
    char s_maxdim[10];
    char s_maxfile[10];   
    char s_rep_file[10];
    char s_nreq[10];
    char s_nclient[10];
    snprintf(s_maxdim, 10, "%d", tolog.maxcapacity_tolog);
    snprintf(s_maxfile, 10, "%d", tolog.maxnumfile_tolog);
    snprintf(s_rep_file, 10, "%d", tolog.replaceop_tolog);
    snprintf(s_nreq, 10, "%d", tolog.clientserved_tolog);
    snprintf(s_nclient, 10, "%d", tolog.maxconnection_tolog);
    /* Concateno alla stringa da loggare */
    strcat(buff,"SERVER INFO: CACHE MAXDIM: ");
    strcat(buff,s_maxdim);
    strcat(buff," MAXNUMFILE: ");
    strcat(buff,s_maxfile);
    strcat(buff," NUMREPLACEDFILE: ");
    strcat(buff,s_rep_file);
    strcat(buff," REQUESTSERVER: ");
    strcat(buff,s_nreq);
    strcat(buff," MAXCONNECTION: ");
    strcat(buff,s_nclient);
}

/* Funzione wrapper per il controllo degli errori della readn */
int d_readn(int source, void* buf, int toread){
    static int i=0;
    int res=0;
    if(res=(readn(source,buf,toread))<0){
        printf("%d",i);
        perror(" readn fallita\n");
        exit(EXIT_FAILURE);
    }
    return res;
}
/* Funzione wrapper per il controllo degli errori della writen */
int d_writen(int source, void* buf, int towrite){
    static int i=0;
    int res=0;
    if(res=(writen(source,buf,towrite))<=0){
        printf("%d",i);
        perror(" writen fallita\n");
        exit(EXIT_FAILURE);
    }
    return res;
}

/* Funzione per inizializzare la struttura per il file di log */
tolog_struct tolog_init(){
    tolog_struct tstruct;
    tstruct.clientserved_tolog=0;
    tstruct.maxcapacity_tolog=0;
    tstruct.maxconnection_tolog=0;
    tstruct.maxnumfile_tolog=0;
    tstruct.replaceop_tolog=0;
    return tstruct;
}
