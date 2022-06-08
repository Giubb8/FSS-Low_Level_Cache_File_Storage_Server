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
#include<dirent.h>
#include<sys/stat.h>
/*Const*/
#define TRUE 1
#define MAXNAME 100
#define MAXARGUMENTLENGHT 256
#define WAIT_CONN_TRY 400 // msec attesa tra un tentativo di connessione e l'altro
#define TIMEOUT 10.0 //secondi prima di timeout connection
#define MAXPATH 100 //lunghezza del path massimo per raggiungere la socket
#define NOTCONNECTED -100
#define O_LOCK 125 //flag o_lock
#define O_CREATE 126 //flag o_create
#define O_BOTH 127 //flag per selezionare entrambi i flag
/*Struct*/
struct sockaddr_un{
    sa_family_t sun_family;
    char sun_path[MAXPATH];
};

/*Flags & globals*/
int f_flag=0;//flag per vedere se socketname settata 2 volte
int p_flag=0;//flag per abilitare le stampe per le operazioni
int x_flag=0;//flag per uscita dal programma
int w_flag=0;//flag per scrittura serve per -D
int r_flag=0;//flag per lettura serve per -d
char socketname[MAXNAME]; //nome della socket
char overload_dir_name[MAXNAME];//nome della directory dove vengono messi i file che provocano overload
int conn_set=0;//flag per vedere se la connessione con il server è attiva 
int wait_time=0;//tempo di attesa tra richieste successive,associato a -t
int fd_socket=-1;//file descriptor della socket
char * separator="~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";
struct sockaddr_un sa;//struct per il soclet address

/*codice per ogni operazione*/
enum opcode_{
    TURNOFF =0,
    OPEN = 1,
    READ = 2,
    READN = 3,
    WRITE = 4,
    REMOVE = 5,
    CLOSE = 6,
    APPEND = 7,
    LOCK = 8,
    UNLOCK = 9,
    
};


/*Funzioni Supporto*/
/* msleep(): Sleep for the requested number of milliseconds. */
int msleep(long msec){
    struct timespec ts;
    int res;

    if (msec < 0){
        errno = EINVAL;
        return -1;
    }
    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;
    do{
        res = nanosleep(&ts, &ts);
    }while(res && errno==EINTR);

    return res;
}


/*  read per evitare scritture parziali */
static int readn(long fd, void *buf, size_t size) {
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
static  int writen(long fd, void *buf, size_t size) {
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
}
