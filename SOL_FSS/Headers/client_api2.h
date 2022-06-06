#include<stdbool.h>
#include<stdio.h>
#include<stddef.h>
#include<stdlib.h>
#include "../Headers/util.h"
//#include "../Headers/client_api.h"



int openConnection(const char* sockname, int msec, const struct timespec abstime){
    if(conn_set==1){//se ho gia una connessione in corso
        perror("connessione gia stabilita con \n");
        errno=ENOENT;
        return -1;
    }
    else{//se connessione puo avvenire
        
        /*preparo le variabili per la connessione*/
        strncpy(sa.sun_path,sockname,SOCKETPATHMAX);
        sa.sun_family=AF_UNIX;
        fd_socket=socket(AF_UNIX,SOCK_STREAM,0);

        /*creo il timer per il timeout*/
        struct timespec now;
        clock_gettime(CLOCK_REALTIME, &now);

        /*provo a connettermi*/
        while( ((connect(fd_socket,(struct sockaddr *)&sa,sizeof(sa)))==-1) && (now.tv_sec < abstime.tv_sec) ){
            if(errno==ENOENT){
                msleep(WAIT_CONN_TRY);
                if(p_flag)printf("attendo %d millisecondi ,riprovo a connettermi\n",WAIT_CONN_TRY);
                clock_gettime(CLOCK_REALTIME, &now); //Ricalcolo il tempo attuale
            }
            else exit(EXIT_FAILURE);
        }
        if(p_flag)printf("%sconnessione con %d effettuata\n",separator,fd_socket);
        return 0;
    }



}
