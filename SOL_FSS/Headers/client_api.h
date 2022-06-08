#include<stdbool.h>
#include<stdio.h>
#include<stddef.h>
#include<stdlib.h>
#include "../Headers/util.h"

/*Struct*/

//struct per rappresentare il messaggio da inviare al server 
typedef struct to_send{
    char filepath[MAXPATH];
    char directory[MAXNAME];
    int o_lock;
    int o_create;
}to_send;


/**
 * Apre una connessione verso il server tramite la socket indicata.
 *
 *  sockname - nome della socket a cui connettersi
 *  msec - intervallo in ms dopo il quale si tenta la riconnessione
 *  abstime - al raggiungimento del tempo di timeout si interrompe il tentativo di connessione
 *  0 se OK, -1 in caso di errore, setta errno opportunamente
 */
int openConnection(const char* sockname, int msec, const struct timespec abstime){
    
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

        
        if(isconnected==0){
            if(p_flag)printf("%sconnessione con %d effettuata\n",separator,fd_socket);
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
int openFile(const char* pathname, int flags){
    if(conn_set==1){
        if(flags==O_LOCK || flags== O_CREATE || flags==O_BOTH){
            int operation=OPEN;
            int pathlen=strlen(pathname);
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
            if( (writen(fd_socket,pathname,pathlen))<= 0){
                perror("writen non riuscita OPEN");
                exit(EXIT_FAILURE);
            }
        }
        else{
            perror("flag non riconosciuto");
            exit(EXIT_FAILURE);
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
int readFile(const char* pathname, void** buf, size_t* size);

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
int appendToFile(const char* pathname, void* buf, size_t size, const char* dirname);

/**
 * Acquisisce la mutua esclusione sul file
 *
 * pathname - file su cui acquisire la mutua esclusione
 * 0 se Ok, -1 in caso di errore, setta errno opportunamente
 */
int lockFile(const char* pathname);


/**
 * Rilascia la mutua esclusione sul file
 *
 * pathname - file su cui rilasciare la mutua esclusione
 * 0 se Ok, -1 in caso di errore, setta errno opportunamente
 */
int unlockFile(const char* pathname);

/**
 * Chiude il file nel server
 *
 * pathname - file da chiudere
 * 0 e Ok, -1 in caso di errore, setta errno opportunamente
 */
int closeFile(const char* pathname);

/**
 * Cancella il file dal server
 * pathname - file da rimuovere
 * se Ok, -1 in caso di errore
 */
int removeFile(const char* pathname);

