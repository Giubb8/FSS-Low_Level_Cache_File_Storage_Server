#include<stdbool.h>
#include<stdio.h>
#include<stddef.h>
#include<stdlib.h>
#include"../Headers/client_api.h"
#include"../Headers/client_globals.h"
#include "../Headers/client_util.h"

/* ######################################## FUNZIONI SUPPORTO  ################################## */


// Wrapper function of the read SysCall handling the interrupted-reading problem
int readn(int source, void* buf, int toread){
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

/* Funzione wrapper per il controllo dell'errore per readn */
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
/* Funzione wrapper per il controllo dell'errore per writen */
int d_writen(int source, void* buf, int towrite){
    static int i=0;
    int res=0;
    if(res=(writen(source,buf,towrite))<0){
        printf("%d",i);
        perror(" writen fallita\n");
        exit(EXIT_FAILURE);
    }
    return res;
}

/* ########################  STRUCTS E GLOBALS   ######################*/


/* socket address*/
struct sockaddr_un{
    sa_family_t sun_family;
    char sun_path[MAXPATH];
};

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
/* codici per gli errori */
enum errors_{
    ERROR = -1,
    FATAL_ERROR = -2,
    SUCCESS =0
};

struct sockaddr_un sa;//struct per il soclet address




/*##########################  IMPLEMENTAZIONI  ###########################################*/
/**
 * Apre una connessione verso il server tramite la socket indicata.
 *
 *  sockname - nome della socket a cui connettersi
 *  msec - intervallo in ms dopo il quale si tenta la riconnessione
 *  abstime - al raggiungimento del tempo di timeout si interrompe il tentativo di connessione
 *  0 se OK, -1 in caso di errore, setta errno opportunamente
 */
int openConnection(const char* sockname, int msec, const struct timespec abstime){
    static int n_op=1;
    if(p_flag)printf("%s %d OPENCONNECTION\n",opseparator,n_op);
    if(conn_set==1){//se ho gia una connessione in corso
        perror("connessione gia stabilita con \n");
        errno=ENOENT;
        return ERROR;
    }
    else{//se connessione puo avvenire

        /*preparo le variabili per la connessione*/
        strncpy(sa.sun_path,sockname,MAXPATH);
        sa.sun_family=AF_UNIX;
        fd_socket=socket(AF_UNIX,SOCK_STREAM,0);
        int isconnected=NOTCONNECTED;

        /*creo il timer per il timeout*/
        float time=0;
        
        /*provo a connettermi*/
        while( ( (isconnected=(connect(fd_socket,(struct sockaddr *)&sa,sizeof(sa)))) !=0 ) && (time < TIMEOUT)){
            if(errno==ENOENT){
                if(p_flag)printf("is connected:%d\nattendo %d millisecondi ,riprovo a connettermi\n",isconnected,WAIT_CONN_TRY);
                msleep(WAIT_CONN_TRY);
                time+=(WAIT_CONN_TRY/1000.0);
            }
            else return ERROR;
        }

        n_op++;
        /* Setto il flag per indicare lo stato della connessione */
        if(isconnected==0){
            if(p_flag)printf("connessione con socketfd %d effettuata\n",fd_socket);
            conn_set=1;
            return SUCCESS;
        }
        else return ERROR;
    }


}

/**
 *  Chiude la connessione verso il server tramite la socket indicata
 *
 *  sockname - nome della socket da cui disconnettersi
 *  0 se Ok, -1 in caso di errore, setta errno opportunamente
 */
int closeConnection(const char* sockname){
    /* Gestione Errore */
    if(close(fd_socket)!=0){
        errno=EPERM;
        perror("errore chiusura socket\n");
        return ERROR;
    }
    else{
        int operation=TURNOFF;
        writen(fd_socket,&operation,sizeof(int));
        if(p_flag)printf("%s CLOSE CONNECTION\n Connessione Chiusa\n%s",opseparator,opseparator);
        return SUCCESS;
    }
}
/**
 * Apre il file sul server
 * pathname - file da aprire
 * flags - modalità di apertura
 * 0 se OK, -1 in caso di errore, setta errno opportunamente
 */
int openFile(char * pathname, int flags){
    static int n_op=1;
    if(p_flag)printf("%s %d OPENFILE \n",opseparator,n_op);
    n_op++;
    if(conn_set==1){
        int replace=-1;
        int reply=-1;
        /* Se non ricevo flags sconosciuti */
        if(flags==O_LOCK || flags== O_CREATE || flags==O_BOTH || flags==NO_FLAG){
            /* Parso il nome del File */
            char pathname_to_parse[MAXPATH];
            strcpy(pathname_to_parse, pathname);
            char filename[MAXNAME];
            parseFilename(pathname_to_parse, filename);
            
            /* Scrivo al Server i dati per gestire l'operazione */
            int operation=OPEN;
            int pathlen=strlen(filename);
            pathlen++;
            fflush(stdout);
            d_writen(fd_socket,&operation,sizeof(int));
            d_writen(fd_socket,&flags,sizeof(int));
            d_writen(fd_socket,&pathlen,sizeof(int));
            d_writen(fd_socket,filename,pathlen);
        }
        else{/* Altrimenti Fallisco */
            errno=EINVAL;
            perror("flag non riconosciuto");
            exit(EXIT_FAILURE);
        }

        /*Caso in cui ho avuto un rimpiazzamento */
        d_readn(fd_socket,&replace,sizeof(int));

        /* Se devo rimpiazzare un file */
        if(replace==1){
            if(p_flag)printf("OPENFILE RIMPIAZZAMENTO\n");
            DIR *dir = NULL; //cartella dove salvare file 
            int file_to_store_len = 0; //lunghezza del file ricevuto
            int file_to_store_name_len = 0; //lunghezza del nome file ricevuto
            unsigned long received_bytes = 0;
            unsigned long stored_bytes = 0;
            char file_to_store_name[MAXNAME]; //nome del file ricevuto 
            char* file_to_store_content = NULL; //contenuto del file ricevuto
            
            /* Se la cartella Esiste la apro */
            if(strlen(overload_dir_name)>0){
                dir = opendir(overload_dir_name);
                if(!dir) {
                    errno=ENOTDIR;
                    perror("APPEND: Errore apertura directory");
                    return ERROR;
                }
            }
        
            /* Leggo la dimensione del file espulso */
            d_readn(fd_socket, &file_to_store_len, sizeof(int));
            /* Se il file non e' vuoto */
            if(file_to_store_len != 0){
                file_to_store_content = malloc(file_to_store_len*sizeof(char));        
                /* leggo  lunghezza nome file, nome file, contenuto */
                d_readn(fd_socket, &file_to_store_name_len, sizeof(int));
                d_readn(fd_socket, file_to_store_name, file_to_store_name_len);
                d_readn(fd_socket, file_to_store_content, file_to_store_len);    
                //printf("filetostorelen: %d filetostorenamelen %d filetostorename: %s filetostorecontent: %s\n",file_to_store_len,file_to_store_name_len,file_to_store_name,file_to_store_content);
                file_to_store_name[file_to_store_name_len] = '\0';
                //Se cartella dirname esiste salvo dentro
                if(overload_dir_name != NULL && dir ) {
                    char fullpath[MAXPATH + MAXNAME] = "";
                    strcpy(fullpath, overload_dir_name);
                    if (fullpath[strlen(fullpath)-1] != '/') strcat(fullpath, "/");
                    strcat(fullpath, file_to_store_name);
                    /* Creo e scrivo il file ricevuto */
                    FILE* file_to_store = fopen(fullpath, "w");
                    if(!file_to_store){
                        perror("Errore creando File");
                        return ERROR;
                    }
                    if (fwrite(file_to_store_content, 1, file_to_store_len, file_to_store) != file_to_store_len){
                        perror("scrittura in locale nel file ");
                        return ERROR;
                    }
                    /* Chiudo il File */
                    if(file_to_store)
                        fclose(file_to_store);
                }
                if(file_to_store_content) free(file_to_store_content);

                if(dir) closedir(dir);
            }
        }
        /* Leggo la risposta del Server */
        d_readn(fd_socket,&reply,sizeof(int));

        if(reply==SUCCESS){
            if(p_flag)printf("file: %s  aperto con successo\n%s\n",pathname,opseparator);
            return SUCCESS;
        }
        else if(reply==EBUSY){
            errno=EBUSY;
            if(p_flag)printf("file: %s non aperto,permessi non acquisiti\n%s\n",pathname,opseparator);
            return EBUSY;
        }
        else{
            errno=EINVAL;
            if(p_flag)printf("file: %s non aperto,fallito\n%s\n",pathname,opseparator);
            return ERROR;
        }
    }
    else{
        errno=EIO;
        perror("connessione non aperta con nessun server");
        exit(EXIT_FAILURE);
    }
    
}

/**
 * Legge il contenuto del file dal server
 *
 * pathname - nome del file da leggere
 * buf - buffer dove memorizzare il contenuto
 * size - dimensione del contenuto ricevuto in bytes
 * 0 se OK, -1 in caso di errore, setta errno opportunamente. In caso di errore buf e size non sono validi
 */
int readFile(const char* pathname, void** buf, size_t* size){
    static int n_op=0;
    n_op++; 
    fflush(stdout);
    
    if(p_flag)printf("%s %d READFILE\n",opseparator,n_op);
    int opcode=READ;
    int pathlen=strlen(pathname);
    int reply=-1;
    /* Scrittura al server dell'operazione */
    d_writen(fd_socket,&opcode,sizeof(int));
    d_writen(fd_socket,&pathlen,sizeof(int));
    d_writen(fd_socket,pathname,pathlen);
    /* Lettura della risposta */
    d_readn(fd_socket,&reply,sizeof(int));
    
    if(reply>0){
        d_readn(fd_socket,size,sizeof(int));
        (*buf)=malloc((int)*size);
        d_readn(fd_socket,*buf, *size);
        return SUCCESS;
    }else{
        errno=EINVAL;
        return ERROR;
    }


}

/**
 * Legge N files dal server
 *
 * N - Numero di file da leggere
 * dirname - eventuale cartella dove memorizzare i file letti
 * 0 se OK, -1 in caso di errore, setta errno opportunamente.
 */
int readNFiles(int N, const char* dirname){
    static int n_op=0;
    n_op++; 
    fflush(stdout);
    if(p_flag)printf("\n%s %d READ_N_FILE\n",opseparator,n_op);
    int opcode=READN;
    int content_size=-1;
    void * content=NULL;
    int filename_len;
    char filename[MAXNAME];
    char pathname[MAXPATH];

    /* mando al server il numero di file da leggere */
    d_writen(fd_socket,&opcode,sizeof(int));    
    d_writen(fd_socket,&N,sizeof(int));
    d_readn(fd_socket,&N,sizeof(int));
    if(p_flag)printf("LEGGENDO %d FILES\n",N);
   
    /* Se la cartella e' stata dichiarata  */
    if(strlen(dirname)>0){
        /* Variabili per la scrittura dentro la cartella */

        DIR *dir = NULL; //cartella dove salvare file 
        char file_to_store_name[MAXNAME] = ""; //nome del file ricevuto 
        char* file_to_store_content = NULL; //contenuto del file ricevuto
        int file_to_store_len = 0; //lunghezza del file ricevuto
        int file_to_store_name_len = 0; //lunghezza del nome file ricevuto
        /* Apro la Cartella se E */
        dir = opendir(dirname);
        if(!dir) {
            errno=ENOTDIR;
            perror("READNFILES: Errore apertura directory");
            return -1;
        }
        /* Creo un ciclo for di n iterazioni */
        for(int i=0;i<N;i++){ 
            
            /*Leggo la dimensione del file che il server mi vuole inviare */
            d_readn(fd_socket,&content_size,sizeof(int));
            
            /*se dimensione e 0 mi fermo */
            if(content_size==0)
                break;

            d_readn(fd_socket,&filename_len,sizeof(int));
            d_readn(fd_socket,filename,filename_len);
            content=malloc(content_size*sizeof(char));       
            /* Leggo il file */
            d_readn(fd_socket,content,content_size);
            fflush(stdout);
            fflush(stderr);
            if(p_flag)printf("\nLetto dal server:\nfile:%s\n%s\n\n",filename,content);
            
            
            /* Se dirname settata,creo il file e scrivo al suo interno */
            if(dirname != NULL && dir ){
                char fullpath[MAXPATH + MAXNAME] = "";
                strcpy(fullpath, dirname);
                if (fullpath[strlen(fullpath)-1] != '/') strcat(fullpath, "/");
                strcat(fullpath, filename);
                /* Creo e scrivo il file ricevuto */
                FILE* file_to_store = fopen(fullpath, "wb");
                if(!file_to_store){
                    perror("Errore READN creando File");
                    return ERROR;
                }
                if (fwrite(content, 1, content_size, file_to_store) != content_size){
                    perror("READN scrittura in locale nel file ");
                    return ERROR;
                }
                /* Chiudo il File */
                if(file_to_store)
                    fclose(file_to_store);
                }
                if(content) free(content);

        }
        if(dir) closedir(dir);
        return SUCCESS;        
    }
    else{/* Altrimenti se la directory non e stata esplicitata */
        for(int i=0;i<N;i++){
            /*Leggo la dimensione del file che il server mi vuole inviare */
            d_readn(fd_socket,&content_size,sizeof(int));
            fflush(stdout);
            if(content_size==0)//se la dimensione e 0 esco dal for 
                break;
            d_readn(fd_socket,&filename_len,sizeof(int));
            d_readn(fd_socket,filename,filename_len);
            content=malloc(content_size*sizeof(char));            
            
            /* Leggo il file */
            d_readn(fd_socket,content,content_size);
                       
            fflush(stdout);
            fflush(stderr);
            /* Stampo il file e libero la memoria */
            if(p_flag)printf("Letto dal server:\nfile:%s\n%s\n\n",filename,content);
            fflush(stdout);
            fflush(stderr); 
            free(content);        
        }
    return SUCCESS;      
    }
          
}

/**
 * Scrive nel server il contenuto del file e salva nella cartella dirname eventuali file espulsi
 *
 * pathname - nome del file da scrivere
 * dirname - cartella in cui salvare eventuali file espulsi
 * 0 se Ok, -1 in caso di errore, setta errno opportunamente
 */
int writeFile(const char* pathname, const char* dirname){
    static int n_op=1;
    if(p_flag)(printf("%s %d WRITEFILE\n",opseparator,n_op));
    n_op++;
    void* buf;
    size_t size;
    if (readFileContent(pathname, &buf, &size) == 0){
        appendToFile(pathname, buf, size, dirname);
        free(buf);
    }
    else{
        errno=EINVAL;
        perror("errore lettura content file\n");
        return ERROR;
    }
    if(p_flag)printf("%s\n",opseparator);
    return SUCCESS;
}

/**
 * Scrive il contenuto dentro buf in aggiunta al file pathname sul server.
 * Salva nella cartella eventuali file espulsi
 *
 * pathname - nome del file a cui aggiungere il contenuto
 * buf - contenuto da aggiungere
 * size - dimensione del contenuto in bytes
 * dirname - cartella in cui salvare eventuali file espulsi
 * 0 se Ok, -1 in caso di errore, setta errno opportunamente
 */
int appendToFile(const char* pathname, void* buf, size_t size, const char* dirname){
    static int n_op=1;
    if(p_flag)(printf("%s %d APPENDFILE\n",opseparator,n_op));
    n_op++;
    /* Inizializzazione */
    DIR *dir = NULL; //cartella dove salvare file 
    int opcode = APPEND; 
    int response=0; //codice di risposta dato dal server 
    int file_to_store_len = 0; //lunghezza del file ricevuto
    int file_to_store_name_len = 0; //lunghezza del nome file ricevuto
    unsigned long received_bytes = 0;
    unsigned long stored_bytes = 0;
    char file_to_store_name[MAXNAME] = ""; //nome del file ricevuto 
    char* file_to_store_content = NULL; //contenuto del file ricevuto
    
    /* Parso il nome del file */
    char pathname_to_parse[MAXPATH];
    strcpy(pathname_to_parse, pathname);
    char filename[MAXNAME];
    parseFilename(pathname_to_parse, filename);

    /* Effettuo Controlli */
    if(strlen(pathname) > MAXPATH ){
        perror("AppendFile: Nome troppo lungo");
        errno = ENAMETOOLONG;
        return ERROR;
    }
    if(dirname != NULL && (strlen(dirname) > MAXPATH)){
        perror("AppendFile: Nome Directory troppo lungo o Inesistente");
        errno = ENAMETOOLONG;
        return ERROR;

    }

    /* Calcolo lunghezza path file + Controlli */
    int pathname_len = strlen(filename);
    pathname_len++;
    if(pathname_len > MAXNAME){
        perror("AppendFile: pathname troppo lungo");
        errno = ENAMETOOLONG;
        return ERROR;
    }
    if(pathname_len <= 0){
        perror("AppendFile: pathname troppo corto");
        errno = EINVAL;
        return ERROR;
    }
    int sizetosend=size;


    /* Invio Dati al Server */
    d_writen(fd_socket, &opcode, sizeof(int));
    d_writen(fd_socket, &pathname_len, sizeof(int));
    d_writen(fd_socket, filename, pathname_len);
    d_writen(fd_socket, &sizetosend, sizeof(int));
    d_writen(fd_socket, buf, size);
     
    /* Aspetto i file esplulsi,se esistono */   
    int replace=0;
    d_readn(fd_socket,&replace,sizeof(int));
    if(replace==1){
        if(strlen(dirname)>0){//se la cartella esiste  la apro
                dir = opendir(dirname);
                if(!dir) {
                    errno=ENOTDIR;
                    perror("APPEND: Errore apertura directory");
                    return ERROR;
                }
        }
    
        /* Leggo la dimensione del file espulso, quando ho finito (0) mi fermo*/
        d_readn(fd_socket, &file_to_store_len, sizeof(int));
        file_to_store_content = malloc(file_to_store_len*sizeof(char));        
        /* leggo  lunghezza nome file, nome file, contenuto */
        d_readn(fd_socket, &file_to_store_name_len, sizeof(int));
        d_readn(fd_socket, file_to_store_name, file_to_store_name_len);
        d_readn(fd_socket, file_to_store_content, file_to_store_len) <= 0;    
        file_to_store_name[file_to_store_name_len] = '\0';
        //Se cartella dirname esiste salvo dentro
        if(dirname != NULL && dir ) {
            char fullpath[MAXPATH + MAXNAME] = "";
            strcpy(fullpath, dirname);
            if (fullpath[strlen(fullpath)-1] != '/') strcat(fullpath, "/");
            strcat(fullpath, file_to_store_name);
            /* Creo e scrivo il file ricevuto */
            FILE* file_to_store = fopen(fullpath, "wb");
            if(!file_to_store){
                perror("Errore creando File");
                return ERROR;
            }
            if (fwrite(file_to_store_content, 1, file_to_store_len, file_to_store) != file_to_store_len){
                perror("scrittura in locale nel file ");
                return ERROR;
            }
            /* Chiudo il File */
            if(file_to_store)
                fclose(file_to_store);
        }
        if(file_to_store_content) free(file_to_store_content);
    
        if(dir) closedir(dir);

        

    }
    
    /* Aspetto risposta dal Server */
    d_readn(fd_socket, &response, sizeof(int));
    if(p_flag)printf("APPEND RESPONSE %d\n",response);
    if(response != 0){
        errno=EINVAL;
        perror("Operazione APPEND non riuscita\n");
        return ERROR;
    }
    else{
        if(p_flag)(printf("Operazione Append %s Avvenuta con Successo\n%s",pathname,opseparator));
        return 0;

    }
    return SUCCESS;
    

}
/**
 * Acquisisce la mutua esclusione sul file
 *
 * pathname - file su cui acquisire la mutua esclusione
 * 0 se Ok, -1 in caso di errore, setta errno opportunamente
 */
int lockFile(const char* pathname){
    static int n_op=1;
    if(p_flag)(printf("%s %d LOCKFILE\n",opseparator,n_op));
    n_op++;
    fflush(stdout);
    if(conn_set==1){
        int opcode=LOCK;
        int ret=0;
        int done=0;
        int filenamelen=0;
        /* Parsing del nome del file*/
        char pathname_to_parse[MAXPATH];
        strcpy(pathname_to_parse, pathname);
        char filename[MAXNAME];
        parseFilename(pathname_to_parse, filename);
        filenamelen=strlen(filename);
        while(done==0){
            /* Invio Operazione al Server */
            d_writen(fd_socket, &opcode, sizeof(int));
            d_writen(fd_socket, &filenamelen, sizeof(int));
            d_writen(fd_socket, filename, filenamelen);
            d_readn(fd_socket, &ret, sizeof(int));
            if(ret==EBUSY){
                if(p_flag)printf("Operazione Lock su %s non avvenuta, lock occupata, riprovo\n",filename);
                usleep(200);
            }
            else{
                if(ret==SUCCESS)
                    if(p_flag)printf("Operazione Lock su %s avvenuta con Successo\n",filename);
                done=1;
            }
        }
        if(p_flag)(printf("%s\n",opseparator));

        return ret;
    }
    else{
        errno=EIO;
        perror("socket non connessa");
        return ERROR;
    }
    
}


/**
 * Rilascia la mutua esclusione sul file
 *
 * pathname - file su cui rilasciare la mutua esclusione
 * 0 se Ok, -1 in caso di errore, setta errno opportunamente
 */
int unlockFile(const char* pathname){
    static int n_op=1;
    if(p_flag)(printf("%s %d UNLOCK FILE\n",opseparator,n_op));
    n_op++;
    if(conn_set==1){
        int opcode=UNLOCK;
        int ret=0;
        int filenamelen=0;
        /* Parsing del nome del file*/
        char pathname_to_parse[MAXPATH];
        strcpy(pathname_to_parse, pathname);
        char filename[MAXNAME];
        parseFilename(pathname_to_parse, filename);
        filenamelen=strlen(filename);
            /* Invio Operazione al Server */
            d_writen(fd_socket, &opcode, sizeof(int));
            d_writen(fd_socket, &filenamelen, sizeof(int));
            d_writen(fd_socket, filename, filenamelen);
            d_readn(fd_socket, &ret, sizeof(int));
            printf("Operazione Unlock su %s avvenuta con Successo\n",filename);

            if(p_flag)(printf("%s\n",opseparator));

            return ret;
    }
    else{
        errno=EIO;
        perror("socket non connessa");
        return ERROR;
    }
        

}


/**
 * Chiude il file nel server
 *
 * pathname - file da chiudere
 * 0 e Ok, -1 in caso di errore, setta errno opportunamente
 */
int closeFile(const char* pathname){
    if(conn_set==1){
        static int n_op=0;
        n_op++;
        if(p_flag)printf("%s %d CLOSEFILE\n",opseparator,n_op);
        /* Inizializzazione */
        int reply=20;        
        int operation=CLOSE;
        
        char pathname_to_parse[MAXPATH];
        strcpy(pathname_to_parse, pathname);
        char filename[MAXNAME];
        parseFilename(pathname_to_parse, filename);
        int pathlen=strlen(filename);
        pathlen++;
        fflush(stdout);

        /* Scrittura al server dell'operazione */
        d_writen(fd_socket,&operation,sizeof(int));
        d_writen(fd_socket,&pathlen,sizeof(int));
        d_writen(fd_socket,filename,pathlen);
        /* Lettura della risposta */
        
        int letti=d_readn(fd_socket,&reply,sizeof(int));

      
        if(reply==SUCCESS){
            if(p_flag)printf("\nfile: %s  chiuso con successo\n%s",pathname,opseparator);
            return SUCCESS;
        }
        else{
            if(p_flag)printf("\nfile %s chiusura,fallita\n%s",pathname,opseparator);
            return ERROR;
        }


    }
    else{
        perror("\nconnessione non aperta con nessun server");
        exit(EXIT_FAILURE);
    }
    
}

/**
 * Cancella il file dal server
 * pathname - file da rimuovere
 * se Ok, -1 in caso di errore
 */
int removeFile(const char* pathname){
if(conn_set==1){
        static int n_op=0;
        n_op++;
        if(p_flag)printf("%s %d REMOVEFILE",opseparator,n_op);
        /* Inizializzazione */
        int reply=-20;        
        int operation=REMOVE;
        
        char pathname_to_parse[MAXPATH];
        strcpy(pathname_to_parse, pathname);
        char filename[MAXNAME];
        parseFilename(pathname_to_parse, filename);
        int pathlen=strlen(filename);
        pathlen++;
        fflush(stdout);

        /* Scrittura al server dell'operazione */
        d_writen(fd_socket,&operation,sizeof(int));
        d_writen(fd_socket,&pathlen,sizeof(int));
        d_writen(fd_socket,filename,pathlen);
        
        /* Lettura della risposta */
        d_readn(fd_socket,&reply,sizeof(reply));


        if(reply==SUCCESS){
            if(p_flag)printf("\nfile: %s  eliminato con successo\n%s",pathname,opseparator);
            return SUCCESS;
        }
        else{
            if(p_flag)printf("\nfile %s eliminazione,fallita\n%s",pathname,opseparator);
            return ERROR;
        }


    }
    else{
        perror("\nconnessione non aperta con nessun server");
        exit(EXIT_FAILURE);
    }    
}

