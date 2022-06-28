#include<stdbool.h>
#include<stdio.h>
#include<stddef.h>
#include<stdlib.h>
#include"../Headers/client_api.h"
#include"../Headers/client_globals.h"
#include "../Headers/client_util.h"



/* ########################  Structs  ######################*/

/* struct per rappresentare il messaggio da inviare al server */
typedef struct to_send{
    char filepath[MAXPATH];
    char directory[MAXNAME];
    int o_lock;
    int o_create;
}to_send;


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
        return -1;
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
            else return -1;
        }

        n_op++;
        if(isconnected==0){
            if(p_flag)printf("connessione con %d effettuata\n",fd_socket);
            conn_set=1;
            return 0;
        }
        else return -1;
    }


}

/**
 *  Chiude la connessione verso il server tramite la socket indicata
 *
 *  sockname - nome della socket da cui disconnettersi
 *  0 se Ok, -1 in caso di errore, setta errno opportunamente
 */
int closeConnection(const char* sockname){
    if(close(fd_socket)!=0){
        perror("errore chiusura socket\n");
        return -1;
    }
    else{
        if(p_flag)printf("%schiusura connessione effettuata%s",separator,separator);
        return 0;
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
    if(p_flag)printf("%s %d OPENFILE\n",opseparator,n_op);
    n_op++;
    if(conn_set==1){
        int reply=-20;
        if(flags==O_LOCK || flags== O_CREATE || flags==O_BOTH || flags==NO_FLAG){
            
            char pathname_to_parse[MAXPATH];
            strcpy(pathname_to_parse, pathname);
            char filename[MAXNAME];
            parseFilename(pathname_to_parse, filename);
            
            int operation=OPEN;
            int pathlen=strlen(filename);
            pathlen++;
            fflush(stdout);
            if( (writen(fd_socket,&operation,sizeof(int)))<= 0){
                perror("writen non riuscita OPEN");
                exit(EXIT_FAILURE);
            }
            if( (writen(fd_socket,&flags,sizeof(int)))<= 0){
                perror("writen non riuscita OPEN");
                exit(EXIT_FAILURE);
            }
            if( (writen(fd_socket,&pathlen,sizeof(int)))<= 0){
                perror("writen non riuscita OPEN");
                exit(EXIT_FAILURE);
            }
            if( (writen(fd_socket,filename,pathlen))<= 0){
                perror("writen non riuscita OPEN");
                exit(EXIT_FAILURE);
            }
        }
        else{
            perror("flag non riconosciuto");
            exit(EXIT_FAILURE);
        }

        if( (readn(fd_socket,&reply,sizeof(reply)))<= 0){
                perror("readn non riuscita OPEN");
                exit(EXIT_FAILURE);
        }
        if(reply==SUCCESS){
            if(p_flag)printf("file: %s  aperto con successo\n%s\n",pathname,opseparator);
            return SUCCESS;
        }
        else{
            if(p_flag)printf("file: %s non aperto,fallito\n%s\n",pathname,opseparator);
            return ERROR;
        }


    }
    else{
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
    if( (writen(fd_socket,&opcode,sizeof(int)))<= 0){
        perror("writen non riuscita OPEN");
        exit(EXIT_FAILURE);
    }
    if( (writen(fd_socket,&pathlen,sizeof(int)))<= 0){
        perror("writen non riuscita OPEN");
        exit(EXIT_FAILURE);
    }
    if( (writen(fd_socket,pathname,pathlen))<= 0){
        perror("writen non riuscita OPEN");
        exit(EXIT_FAILURE);
    }
    printf("Debug 1\n");
    /* Lettura della risposta */
    if( (readn(fd_socket,&reply,sizeof(reply)))<=0){
            perror("readn non riuscita OPEN");
            exit(EXIT_FAILURE);
    }
    printf("Debug 2\n");
    if(reply>0){
        readn(fd_socket,size,sizeof(int));
        (*buf)=malloc((int)*size);
        readn(fd_socket,*buf, *size);
        return SUCCESS;
    }


}

/**
 * Legge N files dal server
 *
 * N - Numero di file da leggere
 * dirname - eventuale cartella dove memorizzare i file letti
 * 0 se OK, -1 in caso di errore, setta errno opportunamente.
 */
int readNFiles(int N, const char* dirname);

/**
 * Scrive nel server il contenuto del file e salva nella cartella dirname eventuali file espulsi
 *
 * pathname - nome del file da scrivere
 * dirname - cartella in cui salvare eventuali file espulsi
 * 0 se Ok, -1 in caso di errore, setta errno opportunamente
 */
int writeFile(const char* pathname, const char* dirname);

/**
 * Scrive il contenuto dentro buf in aggiunta al file pathname sul server.
 * Salva nella cartella eventuali file espulsi
 *
 * pathname - nome del file a cui aggiungere il contenuto
 * uf - contenuto da aggiungere
 * size - dimensione del contenuto in bytes
 * dirname - cartella in cui salvare eventuali file espulsi
 * 0 se Ok, -1 in caso di errore, setta errno opportunamente
 */
int appendToFile(const char* pathname, void* buf, size_t size, const char* dirname){//TODO FREE LE MALLOC
    static int n_op=1;
    if(p_flag)(printf("%s %d APPENDFILE\n",opseparator,n_op));
    n_op++;
    /* Inizializzazione */
    DIR *dir = NULL; //cartella dove salvare file 
    int opcode = APPEND; 
    int response=0; //codice di risposta dato dal server 
    int storedFile_len = 0; //lunghezza del file ricevuto
    int storedFile_name_len = 0; //lunghezza del nome file ricevuto
    unsigned long received_bytes = 0;
    unsigned long stored_bytes = 0;
    char storedFile_name[MAXNAME] = ""; //nome del file ricevuto 
    char* storedFile_data = NULL; //contenuto del file ricevuto
    
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
        return -1;
    }
    if(pathname_len <= 0){
        perror("AppendFile: pathname troppo corto");
        errno = EINVAL;
        return -1;
    }
    int sizetosend=size;


    /* Invio Dati al Server */
    if( writen(fd_socket, &opcode, sizeof(int))<=0){
        perror("writen non riuscita OPEN");
        exit(EXIT_FAILURE);
    }
    if( writen(fd_socket, &pathname_len, sizeof(int))<=0){
        perror("writen non riuscita OPEN");
        exit(EXIT_FAILURE);
    }
    if( writen(fd_socket, filename, pathname_len)<=0){
        perror("writen non riuscita OPEN");
        exit(EXIT_FAILURE);
    }

    if( writen(fd_socket, &sizetosend, sizeof(int))<=0){
        perror("writen non riuscita OPEN");
        exit(EXIT_FAILURE);
    }
    if( writen(fd_socket, buf, size)<=0){
        perror("writen non riuscita OPEN");
        exit(EXIT_FAILURE);
    }
     
    /* Aspetto i file esplulsi,se esistono */   

    if(strlen(dirname)>0){//se la cartella esiste  la apro
        dir = opendir(dirname);
        if(!dir) {
            perror("APPEND: Errore apertura directory");
            return ERROR;
        }
    }
    do{
        /* Leggo la dimensione del file espulso, quando ho finito (0) mi fermo*/
        readn(fd_socket, &storedFile_len, sizeof(int));

        if(storedFile_len == 0) break; //Quando ricevo 0 esco
        storedFile_data = malloc(storedFile_len*sizeof(char));        //TODO CONTROLLO SU STORED DATA controllare che non sia null


        /* leggo  lunghezza nome file, nome file, contenuto */
        readn(fd_socket, &storedFile_name_len, sizeof(int));
        readn(fd_socket, storedFile_name, storedFile_name_len);
        readn(fd_socket, storedFile_data, storedFile_len) <= 0;    

        storedFile_name[storedFile_name_len] = '\0';

        //Se cartella dirname esiste salvo dentro
        if(dirname != NULL && dir ) {
            char complete_path[MAXPATH + MAXNAME] = "";
            strcpy(complete_path, dirname);
            if (complete_path[strlen(complete_path)-1] != '/') strcat(complete_path, "/");
            strcat(complete_path, storedFile_name);
            printf("complete path %s\n",complete_path);


            /* Creo e scrivo il file ricevuto */
            FILE* storedFile = fopen(complete_path, "wb");
            if(!storedFile){
                perror("Errore creando File");
                return ERROR;
            }
            if (fwrite(storedFile_data, 1, storedFile_len, storedFile) != storedFile_len){
                perror("scrittura in locale nel file ");
                return ERROR;
            }
            /* Chiudo il File */
            if(storedFile)
                fclose(storedFile);
        }
        if(storedFile_data) free(storedFile_data);
    }while(true);//BREAK SE LEGGO 0

    if(dir) closedir(dir);

    if(storedFile_data) free(storedFile_data);

    /* Aspetto risposta dal Server */
    readn(fd_socket, &response, sizeof(int));
    if(response != 0){
        perror("Operazione APPEND non riuscita\n");
    }
    else{
        if(p_flag)(printf("Operazione Append Avvenuta con Successo\n%s\n",opseparator));
        return 0;

    }
    

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
    printf("Dentro lockFile\n");
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
            if( writen(fd_socket, &opcode, sizeof(int))<=0){
                perror("writen non riuscita LOCK");
                exit(EXIT_FAILURE);
            }
            if( writen(fd_socket, &filenamelen, sizeof(int))<=0){
                perror("writen non riuscita LOCK");
                exit(EXIT_FAILURE);
            }
            if( writen(fd_socket, filename, filenamelen)<=0){
                perror("writen non riuscita LOCK");
                exit(EXIT_FAILURE);
            }
            if( readn(fd_socket, &ret, sizeof(int))<=0){
                perror("writen non riuscita LOCK");
                exit(EXIT_FAILURE);
            }
            if(ret==EBUSY){
                printf("Operazione Lock su %s non avvenuta, lock occupata, riprovo\n",filename);
                usleep(wait_time);
            }
            else{
                if(ret==SUCCESS)
                    printf("Operazione Lock su %s avvenuta con Successo\n",filename);
                done=1;
            }
        }
        if(p_flag)(printf("%s\n",opseparator));

        return ret;
        

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
    if(p_flag)(printf("%s %d LOCKFILE\n",opseparator,n_op));
    n_op++;
    printf("Dentro unlockFile\n");
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
            if( writen(fd_socket, &opcode, sizeof(int))<=0){
                perror("writen non riuscita UNLOCK");
                exit(EXIT_FAILURE);
            }
            if( writen(fd_socket, &filenamelen, sizeof(int))<=0){
                perror("writen non riuscita UNLOCK");
                exit(EXIT_FAILURE);
            }
            if( writen(fd_socket, filename, filenamelen)<=0){
                perror("writen non riuscita UNLOCK");
                exit(EXIT_FAILURE);
            }
            if( readn(fd_socket, &ret, sizeof(int))<=0){
                perror("writen non riuscita UNLOCK");
                exit(EXIT_FAILURE);
            }
            printf("RET value %d\n",ret);
            if(p_flag)(printf("%s\n",opseparator));

            return ret;
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
        if(p_flag)printf("%s %d CLOSEFILE",opseparator,n_op);
        /* Inizializzazione */
        int reply=-20;        
        int operation=CLOSE;
        
        char pathname_to_parse[MAXPATH];
        strcpy(pathname_to_parse, pathname);
        char filename[MAXNAME];
        parseFilename(pathname_to_parse, filename);
        int pathlen=strlen(filename);
        pathlen++;
        fflush(stdout);

        /* Scrittura al server dell'operazione */
        if( (writen(fd_socket,&operation,sizeof(int)))<= 0){
            perror("writen non riuscita OPEN");
            exit(EXIT_FAILURE);
        }
        if( (writen(fd_socket,&pathlen,sizeof(int)))<= 0){
            perror("writen non riuscita OPEN");
            exit(EXIT_FAILURE);
        }
        if( (writen(fd_socket,filename,pathlen))<= 0){
            perror("writen non riuscita OPEN");
            exit(EXIT_FAILURE);
        }
        
        /* Lettura della risposta */
        if( (readn(fd_socket,&reply,sizeof(reply)))<= 0){
                perror("readn non riuscita OPEN");
                exit(EXIT_FAILURE);
        }


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
int removeFile(const char* pathname);

