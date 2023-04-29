#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/uio.h>

#include "includes/Queue.h"
#include "includes/Conn.h"
void cleanup();



int CreaSocket(){
    // cancello il socket file se esiste, importante da fare all'inizio
    int connfd;
    int listenfd;
    struct sockaddr_un serv_addr;
    cleanup();
    atexit(cleanup);


    // creo il socket
    if((listenfd = socket(AF_UNIX,SOCK_STREAM,0)==-1)){
        perror("socket");
        int errno_copy = errno;
        exit(errno_copy);
    }
    // setto l'indirizzo

    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sun_family = AF_UNIX;
    strncpy(serv_addr.sun_path, SOCKNAME, strlen(SOCKNAME)+1);
    int notused;

    // assegno l'indirizzo al socket
    if((notused = bind(listenfd,(struct sockaddr*)&serv_addr,sizeof(serv_addr))==-1)){
        perror("bind");
        int errno_copy = errno;
        exit(errno_copy);
    }

    // setto il socket in modalita' passiva e definisco un n. massimo di connessioni pendenti
    if((notused = listen(listenfd, SOMAXCONN))==-1){
        perror("bind");
        int errno_copy = errno;
        exit(errno_copy);
    }

    if((connfd = accept(listenfd, (struct sockaddr*)NULL ,NULL))==-1){
        perror("accept ");
        int errno_copy = errno;
        exit(errno_copy);
    }

}




void cleanup() {
    unlink(SOCKNAME);
}
