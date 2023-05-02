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
int CreaSocketClient();


int CreaSocketClient(){
    // cancello il socket file se esiste, importante da fare all'inizio
    int connfd;
    int listenfd;
    int notused;

    struct sockaddr_un serv_addr;
    serv_addr.sun_family = AF_UNIX;
    strncpy(serv_addr.sun_path, SOCKNAME, strlen(SOCKNAME)+1);
    cleanup();
    atexit(cleanup);


    // creo il socket
    if((listenfd = socket(AF_UNIX,SOCK_STREAM,0)==-1)){
        perror("socket");
        int errno_copy = errno;
        exit(errno_copy);
    }
    // setto l'indirizzo


    // assegno l'indirizzo al socket

    listenfd = socket(AF_UNIX,SOCK_STREAM,0);
    notused = bind(listenfd,(struct sockaddr *)&serv_addr,
         sizeof(serv_addr));
    if(notused ==-1){
        perror("bind");
        int errno_copy = errno;
        exit(errno_copy);
    }

    // setto il socket in modalita' passiva e definisco un n. massimo di connessioni pendenti
    if((notused = listen(listenfd, SOMAXCONN))==-1){
        perror("listen");
        int errno_copy = errno;
        exit(errno_copy);
    }

    if((connfd = accept(listenfd, (struct sockaddr*)NULL ,NULL))==-1){
        perror("accept ");
        int errno_copy = errno;
        exit(errno_copy);
    }

    return 1;
}

void cleanup() {
    unlink(SOCKNAME);
}
