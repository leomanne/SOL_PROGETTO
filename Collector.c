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

extern pthread_mutex_t lock;
static int cont = 0;
struct Lista{
    long risultato;
    char* file;
    struct Lista * next;
};
struct Lista* lista=NULL;
void cleanup();
int sort_queue();
int CreaSocketClient();
int cmpfunc (const void * a, const void * b);

void StampaLista();

int CreaSocketClient(){
    // cancello il socket file se esiste, importante da fare all'inizio
    int fd_c;
    int dim;
    int fd_skt;
    int notused;
    long risultato;
    int len;
    struct sockaddr_un serv_addr;
    serv_addr.sun_family = AF_UNIX;
    strncpy(serv_addr.sun_path, SOCKNAME, strlen(SOCKNAME)+1);
    cleanup();
    atexit(cleanup);


    // creo il socket
    if((fd_skt = socket(AF_UNIX,SOCK_STREAM,0)==-1)){
        perror("socket");
        return -1;
    }
    fd_skt = socket(AF_UNIX,SOCK_STREAM,0);
    notused = bind(fd_skt,(struct sockaddr *)&serv_addr,
         sizeof(serv_addr));
    if(notused ==-1){
        perror("bind");
        return -1;
    }

    // setto il socket in modalita' passiva e definisco un n. massimo di connessioni pendenti
    if((notused = listen(fd_skt, SOMAXCONN))==-1){
        perror("listen");
        return -1;
    }

    if((fd_c = accept(fd_skt, (struct sockaddr*)NULL ,NULL))==-1){
        perror("accept ");
        return -1;
    }
    int i = 0;
    while (true){
        if((dim=readn(fd_c,&risultato,sizeof(long)))==-1){
            perror("read in collector");
            return -1;
        }
        if(dim==0){  //fine comunicazione (socket chiusa)
            break;
        }
        //leggo la dimensione della stringa file

        if((dim=readn(fd_c,&len,sizeof(int)))==-1){
            perror("read in collector");
            return -1;
        }

        char* file=NULL;
        file = malloc(sizeof(char)*len+1);
        if((dim = readn(fd_c,file,sizeof(char)*len))==-1){
            perror("read in Collector");
            return -1;
        }
        file[len]= '\0';
        //printf("{il risultato %ld del file %s con lunghezza %d}\n",risultato,file,len);

        struct Lista* lista1=malloc(sizeof(struct Lista));
        lista1->risultato=risultato;
        lista1->file=(char*)malloc(255*sizeof(char));
        strncpy(lista1->file,file,255);
        lista1->next=lista;
        lista=lista1;
        free(file);
        cont++;

        if((dim = writen(fd_c,&i,sizeof(int)))==-1){
            perror("write in collector");
            return -1;
        }


    }
    printf("fine salvataggio in lista\n");
    return 1;
}

void StampaLista() {
    while (lista){
        printf("%s----[%ld]\n",lista->file,lista->risultato);
        lista = lista->next;
    }
}

void cleanup() {
    unlink(SOCKNAME);
}

int cmpfunc (const void * a, const void * b) {
    struct Lista *a1= (struct Lista *)a;
    struct Lista *b1= (struct Lista *)b;
    printf("%ld - %ld\n",a1->risultato,b1->risultato);
    fflush(stdout);
    return (a1->risultato) - b1->risultato;

}
int sort_queue() {
    if (lista == NULL || lista->next == NULL) {
        return -1;
    }

    bool sorted = false;
    while (!sorted) {
        struct Lista* current = lista;
        sorted = true;
        while (current->next) {
            struct Lista* next = current->next;
            if (current->risultato > next->risultato) {
                long temp = current->risultato;
                char filename[MAX_LENGHT_PATH];
                strncpy(filename, current->file, MAX_LENGHT_PATH);
                current->risultato = next->risultato;
                strncpy(current->file, next->file, MAX_LENGHT_PATH);
                next->risultato = temp;
                strncpy(next->file, filename, MAX_LENGHT_PATH);
                sorted = false;
            }
            current = next;
        }
    }

    return 1;
}