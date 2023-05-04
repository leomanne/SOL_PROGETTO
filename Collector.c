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

//-----------------------------------------------

extern pthread_mutex_t lock;

//-----------------------------------------------

static int cont = 0;

struct Lista{
    long risultato;
    char* file;
    struct Lista * next;
};
struct Lista* lista=NULL;

//-----------------------------------------------

void cleanup();
void delete_list();
int sort_queue();
int CreaSocketClient();
void StampaLista();

//-----------------------------------------------

/**
 * Funzione per la gestione della comunicazione con il Master (socket)
 * @return -1 se c'e' stato qualche errore, 1 altrimenti
 */
int CreaSocketClient(){
    // cancello il socket file se esiste, importante da fare all'inizio
    int fd_c;
    int dim;
    int fd_skt;
    int notused;
    long risultato;
    int len;

    cleanup();
    atexit(cleanup);

    struct sockaddr_un serv_addr;
    serv_addr.sun_family = AF_UNIX;
    strncpy(serv_addr.sun_path, SOCKNAME, strlen(SOCKNAME)+1);



    // creo il socket
    if((fd_skt = socket(AF_UNIX,SOCK_STREAM,0))==-1){
        perror("socket");
        return -1;
    }
    //faccio la bind sul socket
    if((notused = bind(fd_skt,(struct sockaddr *)&serv_addr,
                       sizeof(serv_addr))) ==-1){
        perror("bind");
        return -1;
    }

    // setto il socket in modalita' passiva e definisco un n. massimo di connessioni pendenti
    if((notused = listen(fd_skt, SOMAXCONN))==-1){
        perror("listen");
        return -1;
    }
    //in attesa di richieste di connessioni da parte del processo Master
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
    return 1;
}
/**
 * Funzione che stampa i valori nella lista
 */
void StampaLista() {
    struct Lista* tmp = lista;
    while(tmp!=NULL){
        printf("%ld %s\n",tmp->risultato,tmp->file);
        fflush(stdout);
        tmp = tmp->next;
    }
}
/**
 * Funzione usata per pulire dal file "./cs_sock"
 */
void cleanup() {
    unlink(SOCKNAME);
}

/**
 * Funzione per l'ordinamento dei valori nella lista
 * @return -1 se c'e' stato qualche errore , 1 altrimenti
 */
int sort_queue() {
    if (lista == NULL || lista->next == NULL) {
        return -1;
    }
    bool sorted = false; //valore usato come guardia settato a false per eseguire il while almeno una volta

    while (!sorted) {

        struct Lista* current = lista; //current e' il puntatore all'inizio della lista
        sorted = true;
        while (current->next) {//finche il prossimo valore esiste

            struct Lista* next = current->next; //next e' il puntatore al prossimo elemento
            if (current->risultato > next->risultato) { //se il risultato dell'elemento corrente > del successivo devo scambiare
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
/**
 * Funzione per deallocare la memoria della lista
 */
void delete_list() {
    struct Lista* tmp = lista;
    while(lista!=NULL){
        tmp = lista;
        lista = lista->next;
        free(tmp->file);
        free(tmp);
        fflush(stdout);
    }
}