#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include "includes/Queue.h"
#include "includes/Master.h"
#include "includes/Conn.h"
#include "includes/Worker.h"
#define N 100
extern pthread_mutex_t lock;

void cleanup();
int CreaSocketClient();


int CreaSocketClient(){
    // cancello il socket file se esiste, importante da fare all'inizio
    int fd_c;

    int fd_skt;
    int notused;

    struct sockaddr_un serv_addr;
    serv_addr.sun_family = AF_UNIX;
    strncpy(serv_addr.sun_path, SOCKNAME, strlen(SOCKNAME)+1);
    cleanup();
    atexit(cleanup);


    // creo il socket
    if((fd_skt = socket(AF_UNIX,SOCK_STREAM,0)==-1)){
        perror("socket");
        int errno_copy = errno;
        exit(errno_copy);
    }
    // setto l'indirizzo


    // assegno l'indirizzo al socket

    fd_skt = socket(AF_UNIX,SOCK_STREAM,0);
    notused = bind(fd_skt,(struct sockaddr *)&serv_addr,
         sizeof(serv_addr));
    if(notused ==-1){
        perror("bind");
        int errno_copy = errno;
        exit(errno_copy);
    }

    // setto il socket in modalita' passiva e definisco un n. massimo di connessioni pendenti
    if((notused = listen(fd_skt, SOMAXCONN))==-1){
        perror("listen");
        int errno_copy = errno;
        exit(errno_copy);
    }

    if((fd_c = accept(fd_skt, (struct sockaddr*)NULL ,NULL))==-1){
        perror("accept ");
        int errno_copy = errno;
        exit(errno_copy);
    }




    return 1;
}

void cleanup() {
    unlink(SOCKNAME);
}
