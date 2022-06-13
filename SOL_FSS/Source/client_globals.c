
#define MAXPATH 100 //lunghezza del path massimo per raggiungere la socket
#define MAXNAME 100

/*Struct*/

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
