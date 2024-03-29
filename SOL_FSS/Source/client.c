#include"../Headers/client_util.h"
#include"../Headers/client_globals.h"
#include"../Headers/client_api.h"
#include <stdio_ext.h>


#define ERROR -1
#define SUCCESS 0
/* ################### PROTOTIPI HANDLER FUNCTIONS #################### */

void handle_o();
void handle_p();
void handle_h();
void handle_f();
void handle_t();
void handle_D();
void handle_d();
void handle_r();
void handle_x();
void handle_W();
void handle_l();
void handle_u();
void handle_R();
void handle_w();
void handle_c();
int readFile_and_Store();
int isDir();



/* ######################################## MAIN ################################## */


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
    
   /* Controllo Casi p e h E setto i flag*/
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
        if(strcmp(argv[i],"-D") == 0){
            D_flag=1;
        }
        if(strcmp(argv[i],"-d") == 0){
            d_flag=1;
        }
    }

    /*Gestione Input Argv*/
    int opt;
    while( (x_flag==0) && ((opt=getopt(argc,argv,":f:w:W:D:r:R:d:t:l:u:o:c:x"))!=-1) ){
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
                msleep(wait_time);
                break;
                
            case 't':
                handle_t(optarg);
                msleep(wait_time);
                break;
            case 'D':
                handle_D(optarg);
                msleep(wait_time);
                break;
            case 'd':
                handle_d(optarg);
                msleep(wait_time);
                break;
            case 'r':
                handle_r(optarg);
                msleep(wait_time);
                break;
            case 'o':
                handle_o(optarg);
                msleep(wait_time);
                break;
            case 'W':
                handle_W(optarg);
                msleep(wait_time);
                break;
            case 'l':
                handle_l(optarg);
                msleep(wait_time);
                break;
            case 'u':
                handle_u(optarg);
                msleep(wait_time);
                break;
            case 'R':
                handle_R(optarg);
                msleep(wait_time);
                break;
            case 'w':
                handle_w(optarg);
                msleep(wait_time);
                break;
            case 'c':
                handle_c(optarg);
                msleep(wait_time);
                break;
            default:
                break;
            }
    }
    handle_x(socketname);
    return 0;
}


/* ######################################## HANDLERS ################################## */



/* Handler per la scrittura di file singoli*/
void handle_W(char * filesnames){
    if(filesnames==NULL){
        perror("operazione W filesnames null");
        exit(EXIT_FAILURE);
    }
    char* token = strtok(filesnames, ",");
    while (token != NULL) {
        if ( openFile(token,O_BOTH) != 0 ){
            /* Se Fallisco ad Aprire il File con il flag per la creazione riprovo senza */
            if (openFile(token,NO_FLAG) == 0){
                writeFile(token,overload_dir_name);
                closeFile(token);
            }else{
                printf("\nErrore nell'apertura del file %s\n", token);
            }
        }else{
            writeFile(token,overload_dir_name);
            closeFile(token);
        }        
        token = strtok(NULL, ",");
    }
    
}

/* Funzione per Aprire il File */
void handle_o(char * args){
    int flag;
    char* token = strtok(args, ",");
    char * filename=token; 
    token = strtok(NULL, ",");
    /* Setto i flag */
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
    /* Apro il file */
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
        if(p_flag)printf("OpenConnection terminata con successo\n%s\n",opseparator);
    }
    else{
        perror("openConnection terminata con errore\n");
        exit(EXIT_FAILURE);
    }
}


/*funzione per settare tempo richieste successive*/
void handle_t(char * time){
    wait_time=(int)atol(time);
    if(p_flag)printf("%sSET TIME \ntempo per richieste successive settato a %d\n %s",opseparator,wait_time,opseparator);
}


/*funzione che chiude la connessione con la socket*/
void handle_x(char * sock){
    x_flag=1;
    if(conn_set==1){
        if(closeConnection(socketname)!=0){
            exit(EXIT_FAILURE);
        }
    }
}


/*funzione che specifica dove vengono messi i file che provocano overload*/
void handle_D(char * dirpath){
    static int n_op=0;
    n_op++;
    if(p_flag)printf("%s %d OPERAZIONE D\n",opseparator,n_op);
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
        
        int error=mkdir(dirpath, 0777);
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
    printf("%s\n",opseparator);
}


/*cartella dove inserire i file letti dal server*/
void handle_d(char * dirpath){
    static int n_op=0;
    n_op++;

    if(p_flag)printf("%s %d OPERAZIONE d\n",opseparator,n_op);

    /*controllo flag w*/
    if(r_flag==0){
        perror("argomento d usato senza r o R\n");
        exit(EXIT_FAILURE);
    }

    /*se la cartella esiste copio*/
    DIR* dir = opendir(dirpath);//setta il flag in caso di fallimento su ENOENT
    if(dir){
        if(strlen(dirpath)<MAXNAME){//controllo lunghezza
            strcpy(d_overload_dir_name,dirpath);//copio il nome della cartella scelta nella variabile per contenerla
            if(p_flag)printf("nome directory overload: %s\n",d_overload_dir_name);
        }
        else{
            perror("nome troppo lungo");
        }
        closedir(dir);//chiudo la directory
    }
    else if(ENOENT==errno){/* Se la directory non esiste la creo*/
        
        int error=mkdir(dirpath, 0777);
        if(error==0){
            if(strlen(dirpath)<MAXNAME){
                strcpy(d_overload_dir_name,dirpath);
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
    printf("%s",opseparator);

}

/*Invia un messaggio al server per leggere i file passati come argomento*/
void handle_r(char * filesnames){
    char* token = strtok(filesnames, ",");
    while (token != NULL) {
        openFile(token,NO_FLAG);
        if(d_flag){
            readFile_and_Store(token);
        }
        else{
            void * buffer=NULL;
            size_t size;
            readFile(token,&buffer,&size);
            fflush(stdout);
            printf("letti %d caratteri dal server:\n%s\n",strlen(buffer),(char*)buffer);
            fflush(stdout);
            fflush(stderr);
            free(buffer);
        }
        token = strtok(NULL, ",");
        
    }
    if(p_flag)printf("%s",opseparator);

}

/* Handler per lockare un file */
void handle_l(char * filenames){
    char* token = strtok(filenames, ",");
    while (token != NULL) {
        int err=lockFile(token);
        token = strtok(NULL, ",");
    }
}

/* Handler per unlockare un file */
void handle_u(char * filenames){
    char* token = strtok(filenames, ",");
    while (token != NULL) {
        int err=unlockFile(token);
        token = strtok(NULL, ",");
    }
}

/* Handler per la lettura di file multipli */
void handle_R(char * string_num_files){
    /* Prendo dalla stringa il numero di files da leggere dal server */     
    string_num_files=strstr(string_num_files,"=");
    int n=strtol(string_num_files+1,NULL,10);
    readNFiles(n,d_overload_dir_name);
    printf("%s",opseparator);
}


/* Funzione per la visita ricorsiva della cartella Basepath */
void myfilerecursive(char *basePath,int n){
    if(n==0){
        //printf("sto uscendo\n");
        return;
    }
    char path[MAXPATH];
    struct dirent *dp;
    DIR *dir = opendir(basePath);
    int sended=0;
   
    if (!dir)
        return;

    while (((dp = readdir(dir)) != NULL) && sended!=n){
        if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0){
   //         printf("%s\n", dp->d_name);
            strcpy(path, basePath);
            strcat(path, "/");
            strcat(path, dp->d_name);
            if(isDir(path)!=0){
                if ( openFile(path,O_BOTH) != 0 ){
                    if (openFile(path,NO_FLAG) == 0){
                        writeFile(path,overload_dir_name);
                        closeFile(path);
                    }else{
                        printf("\nErrore nell'apertura del file %s\n", path);
                    }
                }else{
                    //printf("Ramo else\n");
                    writeFile(path,overload_dir_name);
                    closeFile(path);
                }        
            n--;
            }
            
            myfilerecursive(path,n);
        }
    }

    closedir(dir);
}

/* Funzione per gestire la Scrittura di una Cartella in maniera ricorsiva */
void handle_w(char * args){
    int n=-1;
    if(strstr(args,"=")!=NULL){
        char * string_num_files=strstr(args,"=");
        n=strtol(string_num_files+1,NULL,10);
    }
    char * token=strtok(args,",");
    myfilerecursive(token,n); 
}

/* Funzione per la cancellazione di un file dal server */
void handle_c(char * filesnames){

    char* token = strtok(filesnames, ",");
    while (token != NULL) {
        removeFile(token);
        token = strtok(NULL, ",");
    }
    if(p_flag)printf("%s",opseparator);
}


/* Funzione per leggere un file nel caso in cui debba rimpiazzare un file per numero massimo di file raggiunti nella cache  */
int readFile_and_Store(char * filepath){
    printf("Dentro Read and store\n");
    
    /* Inizializzazione */
    char * buffer=NULL;//buffer su cui scrivere quanto letto dal server
    size_t size;//dimensione del contentuto letto dla server
    DIR *dir = NULL; //cartella dove salvare file 
    char storedFile_name[MAXNAME] = ""; //nome del file ricevuto 
    char* storedFile_data = NULL; //contenuto del file ricevuto
    int storedFile_len = 0; //lunghezza del file ricevuto
    int storedFile_name_len = 0; //lunghezza del nome file ricevuto

    /* Parsing del nome */
    char pathname_to_parse[MAXPATH];
    strcpy(pathname_to_parse, filepath);
    char filename[MAXNAME];
    parseFilename(pathname_to_parse, filename);

    /* Lettura del contenuto dal server */
    readFile(filename,&buffer,&size);
    fflush(stdout);
    printf("letti %d %d caratteri dal server:\n%s\n",strlen(buffer),(int)size,(char*)buffer);
    /* Apro la Cartella se E */
    if(strlen(d_overload_dir_name)>0){//se la cartella esiste  la apro
        dir = opendir(d_overload_dir_name);
        if(!dir) {
            perror("APPEND: Errore apertura directory");
            return -1;
        }
    }
    /* Se la cartella e' stata settata correttamente */
    if(d_overload_dir_name != NULL && dir ){
        char complete_path[MAXPATH + MAXNAME] = "";
        strcpy(complete_path, overload_dir_name);
        if (complete_path[strlen(complete_path)-1] != '/') strcat(complete_path, "/");
        strcat(complete_path, filename);
        printf("complete path %s\n",complete_path);
        /* Creo e scrivo il file ricevuto */
        FILE* storedFile = fopen(complete_path, "wb");
        if(!storedFile){
            perror("Errore creando File");
            return ERROR;
        }
        if (fwrite(buffer, 1, size, storedFile) != size){
            perror("scrittura in locale nel file ");
            return ERROR;
        }
        
        /* Chiudo il File e libero la memoria  */
        if(storedFile)
            fclose(storedFile);
        }
        if(dir) closedir(dir);
        if(buffer) free(buffer);
        fflush(stdout);
        return SUCCESS;

}


/* Funzione per Controllare se il file è una cartella  */
int isDir(const char* fileName){
    struct stat path;
    stat(fileName, &path);
    return S_ISREG(path.st_mode);
}


/* ######################################## FINE  ################################## */





















