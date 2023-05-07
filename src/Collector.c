#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include "includes/Queue.h"
#include "includes/Conn.h"

//-----------------------------------------------

struct sigaction s;
Lista* lista=NULL;

//-----------------------------------------------

void cleanup();
void delete_list();
int sort_queue();
int CreaSocketServer();
void StampaLista();
int LeggiFile(int fd_c,long risultato);

//-----------------------------------------------

/**
 * Funzione per la gestione della comunicazione con il Master (socket)
 * @return -1 se c'e' stato qualche errore, 1 altrimenti
 */
int CreaSocketServer(){

    int fd_c;
    int dim;
    int fd_skt;
    int notused;
    long risultato = 0;

    // cancello il socket file se esiste, importante da fare all'inizio
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
        if(risultato==-1){
            sort_queue();
            StampaLista();
        }
        if(dim==0){  //fine comunicazione (socket chiusa)
            break;
        }
        if(risultato!=-1) {
            if(LeggiFile(fd_c, risultato)==-1)return -1;
        }

        if((dim = writen(fd_c,&i,sizeof(int)))==-1){
            perror("write in collector");
            return -1;
        }
    }
    return 1;
}
/**
 * Questa funzione legge il nome del file passato dal Master e la aggiunge insieme a @param risultato nella lista
 *
 * @param fd_c file descriptor per il socket usato per la comunicazione
 * @param risultato valore long da recuperato da un certo file. Il file viene recuperato dopo aver preso la lunghezza in bytes di quest'ultimo.
 * @return -1 se c'e' stato qualche errore , 1 altrimenti
 */
int LeggiFile(int fd_c,long risultato) {
    //leggo la dimensione della stringa file
    int dim;
    int len;
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

    Lista* lista1=malloc(sizeof(Lista));
    lista1->risultato=risultato;
    lista1->file=(char*)malloc(MAX_LENGHT_PATH*sizeof(char));
    strncpy(lista1->file,file,MAX_LENGHT_PATH);
    lista1->next=lista;
    lista=lista1;
    //printf("inserito [%s]\n",lista1->file);
    free(file);
    return 1;
}

/**
 * Funzione che stampa i valori nella lista
 */
void StampaLista() {
    Lista* tmp = lista;
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
 * Funzione per deallocare la memoria della lista
 */
void delete_list() {
    Lista* tmp = lista;
    while(lista!=NULL){
        tmp = lista;
        lista = lista->next;
        free(tmp->file);
        free(tmp);
        fflush(stdout);
    }
}
/**
 *
 * @param a primo elemento da scambiare della lista
 * @param b secondo elemento da scambiare della lista
 */
void swap(Lista* a, Lista* b) {
    long temp_risultato = a->risultato;
    char* temp_file = a->file;

    a->risultato = b->risultato;
    a->file = b->file;

    b->risultato = temp_risultato;
    b->file = temp_file;
}

/**
 * Funzione usata per fare il sorting della lista
 * @return  -1 se la lista e' vuota, 1 altrimenti
 */
int sort_queue() {
    Lista* current = lista;
    Lista* index = NULL;

    if (lista == NULL)
        return -1;
    while (current != NULL) {
        index = current->next;
        while (index != NULL) {
            if (current->risultato > index->risultato) {
                swap(current, index);
            }
            index = index->next;
        }
        current = current->next;
    }
    return 1;
}

