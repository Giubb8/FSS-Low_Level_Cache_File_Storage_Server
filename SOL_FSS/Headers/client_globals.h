#pragma once

#include <sys/socket.h>
#define MAXPATH 100 //lunghezza del path massimo per raggiungere la socket
#define MAXNAME 100


extern int f_flag;//flag per vedere se socketname settata 2 volte
extern int p_flag;//flag per abilitare le stampe per le operazioni
extern int x_flag;//flag per uscita dal programma
extern int w_flag;//flag per scrittura serve per -D
extern int r_flag;//flag per lettura serve per -d
extern char socketname[MAXNAME]; //nome della socket
extern char overload_dir_name[MAXNAME];//nome della directory dove vengono messi i file che provocano overload
extern int conn_set;//flag per vedere se la connessione con il server è attiva 
extern int wait_time;//tempo di attesa tra richieste successive,associato a -t
extern int fd_socket;//file descriptor della socket
extern char * separator;
