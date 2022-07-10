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
#define NO_FLAG 128 //flag nullo

/*Funzioni Supporto*/

/* msleep(): Sleep for the requested number of milliseconds. */
static int msleep(long msec){
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
/*static int readn(long fd, void *buf, size_t size) {
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
}*/


/*  writen per evitare scritture parziali 3e parti*/ 
/*static  int writen(long fd, void *buf, size_t size) {
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

/* Legge il contenuto di un file */
static int readFileContent(char* pathname, void** buf, size_t* size){
    if(!pathname || !buf || !size) {
        fprintf(stderr, "\n1Client: errore lettura contenuto file %s. Info sull'errore: %s\n", pathname,
                strerror(errno));
        errno = EINVAL;
        return -1;
    }
    FILE* file;
    file = fopen(pathname, "rb+");
    if(!file){
        fprintf(stderr, "\n2Client: errore lettura contenuto file %s. Info sull'errore: %s\n", pathname,
                strerror(errno));
        return -1;
    }
    struct stat sb;
    if (stat(pathname, &sb) == -1){
        fprintf(stderr, "\n3Client: errore lettura statistiche file %s. Info sull'errore: %s\n", pathname,
                strerror(errno));
        return -1;
    }
    *size = sb.st_size;
    if(*size <= 0){
        errno = ENODATA;
        fprintf(stderr, "\n4Client: errore lettura contenuto file %s. Info sull'errore: %s\n", pathname,
                strerror(errno));
        return -1;
    }

    *buf = malloc(*size);
    if(!*buf){
        perror("\nreadFileContent: Errore allocazione memoria\n");
        exit(EXIT_FAILURE);
    }
    /*while (fread(*buf, 1, *size, file) > 0){

    }*/
    fread(*buf, 1, *size, file);
    fclose(file);
    return 0;
}

/* Effettua il parsing del nome di un file a partire dal path */
static inline void parseFilename(char* pathname, char* result){
    char* filename;
    char name[MAXNAME];
    char* rest = NULL;
    filename = strtok_r(pathname, "/", &rest);
    while(filename != NULL){
        strcpy(name, filename);
        filename = strtok_r(NULL, "/", &rest);
    }
    strcpy(result, name);
}
