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
    
    printf("Handling segnale SIGHINTQUIT\n");
    sighintquit=1;
    shutdown(fd_socket,SHUT_RD);
    return;
}

//handling sighup
void sig_sighup_handler(){
    printf("Handling segnale SIGHUP\n");
    sighup=1;
    shutdown(fd_socket,SHUT_RD);
    return;
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
/*  read per evitare scritture parziali */
/*int readn(long fd, void *buf, size_t size) {
    size_t left = size;
    int r;
    char *bufptr = (char*)buf;
    while(left>0) {
	if ((r=read((int)fd ,bufptr,left)) == -1) {
	    if (errno == EINTR) continue;
	    return -1;
	}
	if (r == 0) return 0;   // EOF
        left    -= r;
	bufptr  += r;
    }
    return size;
}


/*  writen per evitare scritture parziali 3e parti*/ 
/*int writen(long fd, void *buf, size_t size) {
    size_t left = size;
    int r;
    char *bufptr = (char*)buf;
    while(left>0) {
	if ((r=write((int)fd ,bufptr,left)) == -1) {
	    if (errno == EINTR) continue;
	    return -1;
	}
	if (r == 0) return 0;  
        left    -= r;
	bufptr  += r;
    }
    return 1;
}*/

void print_op(int opcode){
    static int n_op=1;
    printf("%d ",n_op);
    switch (opcode){
    case 0:
        printf("SERVER TURNOFF\n");
        break;
    case 1:
        printf("SERVER OPEN\n");
        break;
    case 2:
        printf("SERVER READ\n");
        break;
    case 3:
        printf("SERVER READN\n");
        break;
    case 4:
        printf("SERVER WRITE\n");
        break;
    case 5:
        printf("SERVER REMOVE\n");
        break;
    case 6:
        printf("SERVER CLOSE\n");
        break;
    case 7:
        printf("SERVER APPEND\n");
        break;
    case 8:
        printf("SERVER LOCK\n");
        break;
    case 9:
        printf("SERVER UNLOCK\n");
        break;
    default:
        break;
    }
    n_op++;
    
    return;
    
}
void print_flag(int flag){
    switch (flag){
    case O_BOTH:
        printf("FLAG OBOTH\n");
        break;
    case NO_FLAG:
        printf("FLAG NOFLAG\n");
        break;
    case O_CREATE:
        printf("FLAG OCREATE\n");
        break;
    case O_LOCK:
        printf("FLAG OLOCK\n");
        break;

    default:
        break;
    }
}

void* logger_func(void* arg) {
    int temperr;
    int logfile_fd;
    char* buffer=NULL;
    fflush(stdout);
    while(sighintquit==0 && sighup==0) {
        fflush(stdout);

        //locko la queue 
        m_lock(&(log_queue->queue_mtx));
        //controllo se sono stati inseriti nuovi elementi
        while(!(buffer=(char*)conc_queue_pop(log_queue)) && (sighintquit==0)) {
            m_wait(&(log_queue->queue_cv), &(log_queue->queue_mtx));
        }
    fflush(stdout);

        m_unlock(&(log_queue->queue_mtx));
        if(sighintquit) break;   // server termina

        if((logfile_fd=open(LOGFILE_PATH, O_WRONLY | O_APPEND | O_CREAT, 0777))==ERROR) {
            LOG_ERR(errno, "logger: aprendo il file");
            exit(EXIT_FAILURE);
        }

        writen(logfile_fd, (void*)buffer, strlen(buffer));
        
        if((close(logfile_fd))==ERROR) {
            LOG_ERR(errno, "logger: chiudendo il file ");
            exit(EXIT_FAILURE);
        }


        if(buffer) {free(buffer); buffer=NULL;}
    }
    printf("uscito da logger function sighint==%d\n",sighintquit);
    fflush(stdout);
    fflush(stderr);
    
    if(buffer)
        free(buffer);
    pthread_exit((void*)1);
}

/* Funzione per effettuare il log delle statistiche relative alla cache */
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






