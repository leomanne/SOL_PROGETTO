#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <bits/types/sig_atomic_t.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include "includes/Queue.h"
#include "includes/Worker.h"
#include "includes/Master.h"
#include "includes/Collector.h"
#include "includes/Conn.h"
//-----------------------------------------

#define NTHREAD 4
#define QLEN 8
#define DELAY 0
#define EOS (void*)0x1

//-----------------------------------------

Queue *q;
volatile int finished_insert=0;
int fc_skt;
pthread_mutex_t lock= PTHREAD_MUTEX_INITIALIZER; //per scrivere / leggere in mutua esclusione sulla socket

//-----------------------------------------

int isNumber(const char *s, int *n);
int setNThread(const char *m, int *n);
int setQlen(const char *m, int *n);
void printUsage();
int setDelay(char *optarg, int *pInt);

//-----------------------------------------
int main(int argc, char *argv[]) {
    int pid;
    /*usati per le informazioni di default */
    int nthread = NTHREAD;
    int qlen = QLEN;
    int delay = DELAY;

    bool argd = false;// flag per la directory passata

    if (argc == 1) {
        printf("almeno una opzione deve essere passata\n");
        printUsage();
        return -1;
    }
    int opt;
    char *tmp = NULL;
    /*controllo i valori passati da riga di comando*/
    while ((opt = getopt(argc, argv, ":n:q:t:d:")) != -1) {
        switch (opt) {
            case 'n':
                if (setNThread(optarg, &nthread) == -1) { //setta il numero di thread
                    printUsage();
                    exit(0);
                }
                //printf("nthreads = [%d]", nthread);
                break;
            case 'q':
                if (setQlen(optarg, &qlen) == -1) { //setta la lunghezza della coda concorrente
                    printUsage();
                    exit(0);
                }
                break;
            case 't':
                if (setDelay(optarg, &delay) == -1) {//setta la quantita di millisecondi da aspettare per aggiungere un valore alla coda concorrente
                    printUsage();
                    exit(0);
                }
                break;
            case 'd': //setto il valore directory passata = true
                tmp = optarg;
                argd = true;
                break;
            case ':': {
                printf("l'opzione '-%c' richiede un argomentoo\n", optopt);
                printUsage();
                exit(1);
            }
                break;
            case '?': {  // restituito se getopt trova una opzione non riconosciuta
                printf("l'opzione '-%c' non e' gestita\n", optopt);
                printUsage();

            }
            break;
            default:;
        }
    }

    //duplico il processo
    pid=fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {  //Gestione collector

        int t = CreaSocketClient(); //gestione socket da parte del collector
        if(t == -1){
            printf("errore in Collector\n");
        }else {
            t = sort_queue(); //ordino la lista di elementi ricevuta dai worker
            if(t == -1) {
                printf("la lista era vuota\n");
            }
            StampaLista(); //stampo la lista di valori <risultato,nomefile>

        }
        delete_list();// dealloca la memoria
    } else {  //Gestione Master
        q = initQueue(qlen);//inizializzo la coda concorrente
        if (!q) {
            fprintf(stderr, "initBQueue fallita\n");
            exit(errno);
        }

        //struttura dati usata dal metodo Insert
        infoInsert info;
        info.tmp = tmp;
        info.argc = argc;
        info.argv = argv;
        info.argd = argd;
        info.q = &q;
        info.delay = delay;
        info.nthreads= nthread;

        pthread_t* pool = (pthread_t*)malloc(sizeof (pthread_t)*nthread);//alloco i thread workers
        if (!pool) {
            fprintf(stderr, "malloc fallita\n");
            return EXIT_FAILURE;
        }

        fc_skt = CreaSocketServer();//gestione Socket da parte del master
        if(fc_skt==-1) { //se e' stato rilevato un errore libero la memoria ed esco
            free(pool);
            deleteQueue(q, NULL);
            return 1;
        }
        for(int i=0;i<nthread;i++){//creo tutti i thread passandogli la coda concorrente
            if((pthread_create(&pool[i],NULL,worker,q))!=0){
                perror("pthread_create");
                exit(EXIT_FAILURE);
            }
        }
        Insert(&info);//Inserimento dei file nella coda
                      //Importante che l'inserimento venga fatto dopo la creazione dei workers, push e pop sono bloccanti.

        for (int i = 0; i < nthread; ++i) {
            if (pthread_join(pool[i], NULL) == -1) {
                fprintf(stderr, "pthread_join failed\n");
            }
        }
        /*chiudo il socket*/
        shutdown(fc_skt,SHUT_RDWR);
        if(close(fc_skt)==-1){
            perror("close socket");
            exit(EXIT_FAILURE);
        }

        waitpid(pid,NULL,0);//attendo la terminazione del collector
        if(pool)free(pool);//dealloco la memoria per i thread che sono terminati
        deleteQueue(q, NULL);//dealloco la memoria per la coda

    }
    return 0;
}

/**
 *
 * @param optArg e' il valore intero char* da parsare in int
 * @param n e' il valore delay da modificare
 * @return 0 se e' andato tutto bene , -1 altrimenti
 */
int setDelay(char *optArg, int *n) {
    int tmp;
    if (isNumber(optArg, &tmp) != 0) {
        printf("l'argomento di '-n' non e' valido\n");
        return -1;
    }
    //printf("-t : %d\n", tmp);
    *n = tmp;
    return 0;
}
/**
 * @effect stampa lo usage di farm
 */
void printUsage() {
    printf("usage :./<name_program> [-n] <value> [-q] <value> [-t] <value> [-d] <pathname_to_dir> <pathname_to_file> ...\n");
    fflush(stdout);
}
/**
 *
 * @param optArg e' il valore intero char* da parsare in int
 * @param n e' il valore n_thread da modificare
 * @return 0 se e' andato tutto bene , -1 altrimenti
 */
int setNThread(const char *optArg, int *n) {
    int tmp;
    if (isNumber(optArg, &tmp) != 0) {
        printf("l'argomento di '-n' non e' valido\n");
        return -1;
    }
   // printf("-n : %d\n", tmp);
    *n = tmp;
    return 0;
}
/**
*
* @param optArg e' il valore intero char* da parsare in int
* @param n e' il valore qLen da modificare
* @return 0 se e' andato tutto bene , -1 altrimenti
*/
int setQlen(const char *optArg, int *n) {
    int tmp;
    if (isNumber(optArg, &tmp) != 0) {
        printf("l'argomento di '-q' non e' valido\n");
        return -1;
    }
    //printf("-q : %d\n", tmp);
    *n = tmp;
    return 0;
}
/**
 *
 * @param s stringa da parsare in int
 * @param n valore da modificare
 * @return 0 se e' andato tutto bene, 1 altrimenti
 */
int isNumber(const char *s, int *n) {
    if (s == NULL) return 1;
    if (strlen(s) == 0) return 1;
    char *e = NULL;
    errno = 0;
    long val = strtol(s, &e, 10);
    if (errno == ERANGE) return 2;    // overflow
    if (e != NULL && *e == (char) 0) {
        *n = val;
        return 0;   // successo
    }
    return 1;   // non e' un numero
}



