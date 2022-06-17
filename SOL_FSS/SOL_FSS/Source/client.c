#include"../Headers/client_util.h"
#include"../Headers/client_globals.h"
#include"../Headers/client_api.h"

/* ################### HANDLER FUNCTIONS #################### */

void handle_o();
void handle_p();
void handle_h();
void handle_f();
void handle_t();
void handle_D();
void handle_d();
void handle_r();
void handle_x();


/* ################### MAIN ################################## */


int main(int argc, char *argv[]){
    if(argc<1){
            perror("input insufficienti");
            exit(EXIT_FAILURE);
        }

    

    /*Variabili Interne Client*/
    struct timespec timer;
    if( clock_gettime(CLOCK_REALTIME, &timer) == -1)
        exit(EXIT_FAILURE);
    timer.tv_sec += TIMEOUT;
    
   /* Controllo Casi p e h*/
    for(int i = 0; i < argc; i++){
        if(strlen(argv[i])>MAXARGUMENTLENGHT) {
            printf("\nArgomento %d non rispetta il limite di lunghezza\n", i);
            exit(EXIT_FAILURE);
        }
        if(strcmp(argv[i], "-p") == 0)
            handle_p();
        if(strcmp(argv[i], "-h") == 0) {
            handle_h();
            exit(EXIT_SUCCESS);
        }
        if(strcmp(argv[i],"-w") == 0){
            w_flag=1;
        }
        if(strcmp(argv[i],"-W") == 0){
            w_flag=1;
        }
        if(strcmp(argv[i],"-r") == 0){
            r_flag=1;
        }
        if(strcmp(argv[i],"-R") == 0){
            r_flag=1;
        }
    }

    /*Gestione Input Argv*/
    int opt;
    while( (x_flag==0) && ((opt=getopt(argc,argv,":f:w:W:D:r:R:d:t:l:u:o:c:px"))!=-1) ){
        switch(opt){
            case 'f':
                if(f_flag==1){
                    perror("socketname già settata\n");
                    exit(EXIT_FAILURE);
                }
                else{
                    if(strlen(optarg)<MAXNAME){//se nome va bene
                        f_flag=1;
                        strncpy(socketname,optarg,sizeof(optarg));
                        if(p_flag)printf("nome socket ricevuto correttamente %s\n",socketname);
                        handle_f(socketname,timer);
                    }
                    else{
                        perror("lunghezza nome socket non idonea\n");
                        exit(EXIT_FAILURE);
                    }
                }
                break;
            case 'x':
                handle_x(socketname);
                
            case 't':
                handle_t(optarg);
            case 'D':
                handle_D(optarg);
            case 'd':
                handle_d(optarg);
            case 'r':
                handle_r(optarg);
            case 'o':
                handle_o(optarg);
            default:
                break;
            }
    }
    return 0;
}

void handle_o(char * args){
    int flag;
    char* token = strtok(args, ",");
    char * filename=token; 
    token = strtok(NULL, ",");
    if(strcmp(token,"O_LOCK")==0){
        flag=O_LOCK;
    }
    if(strcmp(token,"O_CREATE")==0){
        flag=O_CREATE;
    }
    if(strcmp(token,"O_BOTH")==0){
        flag=O_BOTH;
    }
    if(strcmp(token,"NO_FLAG")==0){
        flag=NO_FLAG;
    }
    openFile(filename,flag);

}

/*abilita le stampe per ogni operazione*/
void handle_p(){
    p_flag=1;
    printf("settato flag p,stampe abilitate\n");
}


/*elenca i comandi dispobili per ogni client*/
void handle_h(){
     printf(" HELPER:\n\
     Comandi:\n\
    -f filename: specifica il nome del socket a cui connettersi \n\
    -w dirname[,n=0]: invia al server al massimo 'n' file nella cartella ‘dirname’, se n=0 viene inviato tutto il contenuto della cartella \n\
    -W file1[,file2]: lista di nomi di file da scrivere nel server separati da ‘,’ \n\
    -D dirname: cartella dove ricevere i file che il server rimuove per spazio insufficiente\n\
    -r file1[,file2]: lista di nomi di file da leggere dal server separati da ‘,’\n\
    -R [n=0]: legge ‘n’ file qualsiasi attualmente memorizzati nel server, se n=0 o non specificato allora vengono letti tutti i file presenti nel server\n\
    -d dirname: cartella dove scrivere i file letti dal server da usare dopo ‘-r’ o ‘-R’\n\
    -t time: tempo in che intercorre tra l’invio di due richieste da parte del client \n\
    -l file1[,file2]: lista di nomi di file su cui acquisire la lock\n\
    -u file1[,file2]: lista di nomi di file su cui rilasciare la lock\n\
    -c file1[,file2]: lista di file da rimuovere dal server \n\
    -p: abilita le stampe sullo standard output \n   ");
} 


/*apre la connessione con il server al nome passato come argomento e gestisce errori*/
void handle_f(char * socketname,struct timespec timer){
    timer.tv_sec=TIMEOUT;
    int err=openConnection(socketname,WAIT_CONN_TRY,timer);
    if(err==0){
        if(p_flag)printf("OpenConnection terminata con successo\n");
    }
    else{
        perror("openConnection terminata con errore\n");
        exit(EXIT_FAILURE);
    }
}


/*funzione per settare tempo richieste successive*/
void handle_t(char * time){
    wait_time=(int)atol(time);
    if(p_flag)printf("tempo per richieste successive settato a %d\n",wait_time);
}


/*funzione che chiude la connessione con la socket*/
void handle_x(char * sock){
    x_flag=1;
    if(p_flag)printf("caso x ricevuto,termino\n");
    if(conn_set==1){
        if(closeConnection(socketname)!=0){
            exit(EXIT_FAILURE);
        }
    }
}


/*funzione che specifica dove vengono messi i file che provocano overload*/
void handle_D(char * dirpath){
   
    /*controllo flag w*/
    if(w_flag==0){
        perror("argomento D usato senza w o W\n");
        exit(EXIT_FAILURE);
    }

    /*se la cartella esiste copio*/
    DIR* dir = opendir(dirpath);
    if(dir){
        if(strlen(dirpath)<MAXNAME){//controllo lunghezza
            strcpy(overload_dir_name,dirpath);
            if(p_flag)printf("nome directory overload ricevuto\n");
        }
        else{
            perror("nome troppo lungo");
        }
        closedir(dir);//chiudo la directory
    }
    else if(ENOENT==errno){/* directory non esiste*/
        
        int error=mkdir(dirpath, 0777);//TODO CONTROLLARE SE MASK OK
        if(error==0){
            if(strlen(dirpath)<MAXNAME){
                strcpy(overload_dir_name,dirpath);
                if(p_flag)printf("directory creata e nome directory overload ricevuto\n");
            }
            else{
                perror("nome troppo lungo");
            }
        }
        else{
            perror("errore creazione directory\n");
        } 
    }
    else{
        perror("errore durante handle_D");
        exit(EXIT_FAILURE);
    } 
}


/*cartella dove inserire i file letti dal server*/
void handle_d(char * dirpath){
   
    /*controllo flag w*/
    if(r_flag==0){
        perror("argomento D usato senza r o R\n");
        exit(EXIT_FAILURE);
    }

    /*se la cartella esiste copio*/
    DIR* dir = opendir(dirpath);
    if(dir){
        if(strlen(dirpath)<MAXNAME){//controllo lunghezza
            strcpy(overload_dir_name,dirpath);//copio il nome della cartella scelta nella variabile per contenerla
            if(p_flag)printf("nome directory overload ricevuto\n");
        }
        else{
            perror("nome troppo lungo");
        }
        closedir(dir);//chiudo la directory
    }
    else if(ENOENT==errno){/* directory non esiste*/
        
        int error=mkdir(dirpath, 0777);//TODO CONTROLLARE SE MASK OK
        if(error==0){
            if(strlen(dirpath)<MAXNAME){
                strcpy(overload_dir_name,dirpath);
                if(p_flag)printf("directory creata e nome directory overload ricevuto\n");
            }
            else{
                perror("nome troppo lungo");
            }
        }
        else{
            perror("errore creazione directory\n");
        } 
    }
    else{
        perror("errore durante handle_d");
        exit(EXIT_FAILURE);
    } 
}

/*Invia un messaggio al server per leggere i file passati come argomento*/
void handle_r(char * filesnames){
    
    char* token = strtok(filesnames, ",");
    while (token != NULL) {
        printf("dentro r %s\n", token);
        openFile(token,O_CREATE);
        token = strtok(NULL, ",");
        //readfile
    }
}