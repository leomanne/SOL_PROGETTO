#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include "includes/Queue.h"
#include "includes/Conn.h"

//-----------------------------------------------

#define EOS (void*)0x1
#define MAX_LENGHT_PATH 255

//-----------------------------------------------

extern pthread_mutex_t lock;
extern int fc_skt;

//-----------------------------------------------

int SendMsgToCollector(char *args, long result);


/**
 * funzione eseguita dal Worker thread, prende il valore dalla coda, calcola il valore e lo comunica al collector
 * @param queue coda concorrente
 * @return
 */
void * worker(void *queue) {
    Queue *q = (Queue *) queue;
    long result;
    int r;
    while (true) {
        result = 0;
        bool guard = false;
        char *args = pop(q);

        if (args == EOS) {
            return NULL;
        }else{
            struct stat statbuf;
            if ((r = stat(args, &statbuf)) == -1) {
                perror("Facendo stat del file");
                printf("{%s}\n",args);
                return NULL;
            }
            if(!S_ISREG(statbuf.st_mode)){
                free(args);
                break;
            }
            FILE *f;
            long i = 0;
            long tmp;
             if ((f = fopen(args, "rb")) == NULL) {
                 printf("fallito [%s]", args);
                 perror("open file");
                 //return NULL; non termino il thread
             }

             long x = 0;
             while (fread(&tmp, sizeof(long), 1, f)) {
                 x = result;
                 result = result + (i * tmp);
                 if(x > result) guard= true;
                 i++;
             }
             fclose(f);
             fflush(stdout);
            }
            if(!guard) {
                result = SendMsgToCollector(args, result);
            }else{
                //printf("Errore valore overflow: %s non inserito\n",args);
                free(args);
            }
    }
    return NULL;
}
/**
 * manda dei messaggi al collector: il risultato del file , la lunghezza in bytes della stringa args e infine args.
 * infine attende un riscontro dal collector
 * @param args stringa con il nome del file da mandare
 * @param result valore elaborato dal worker del file args
 * @return -1 se qualcosa e' andato storto, 0 altrimenti
 */
int SendMsgToCollector(char *args, long result){

    int len=strlen(args);
    int n;
    int tmp;
    long risposta;

    pthread_mutex_lock(&lock);
    if((n=writen(fc_skt,&result,sizeof(long)))==-1){
        perror("writen result");
        pthread_mutex_unlock(&lock);
        return -1;
    }

    if((n=writen(fc_skt,&len,sizeof(int)))==-1){
        perror("writen len file");

        pthread_mutex_unlock(&lock);
        return -1;
    }
    //printf("sto per scrivere %s\n",args);
    if((n=writen(fc_skt,args,sizeof(char)*len))==-1){
        perror("writen file");

        pthread_mutex_unlock(&lock);
        return -1;
    }
    //vedo se il collector ha finito
    if((tmp=readn(fc_skt,&risposta,sizeof(int)))==-1){
        perror("read in sendCollector ");
        pthread_mutex_unlock(&lock);
        pthread_mutex_unlock(&lock);
        return -1;
    }
    free(args);
    pthread_mutex_unlock(&lock);

    return 0;
}
