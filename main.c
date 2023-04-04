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
#include "Queue.h"
#include "Threadpool.h"
#include "Worker.h"
#include "Master.h"

#define NTHREAD 4
#define QLEN 8
#define DELAY 0
#define MAX_LENGHT_PATH 255
#define EXIT_MSG "-exit-"
#define EOS (void*)0x1
Queue *q;
typedef struct threadArgs {
    int thid;
    Queue *q;
} threadArgs_t;

int isNumber(const char *s, int *n);

int setNThread(const char *m, int *n);

int setQlen(const char *m, int *n);


void printUsage();



int setDelay(char *optarg, int *pInt);

void *Consumer(void *arg);

int main(int argc, char *argv[]) {
    int nthread = NTHREAD;
    int qlen = QLEN;
    int pid;
    int delay = DELAY;
    bool argd = false;
    //char *nameDir;
    if (argc == 1) {
        printf("almeno una opzione deve essere passata\n");
        printUsage();
        return -1;
    }
    int opt;

    char *tmp;
    while ((opt = getopt(argc, argv, ":n:q:t:d:")) != -1) {
        switch (opt) {
            case 'n':
                if (setNThread(optarg, &nthread) == -1) {
                    printUsage();
                    exit(0);
                }
                printf("nthreads = [%d]", nthread);
                break;
            case 'q':
                if (setQlen(optarg, &qlen) == -1) {
                    printUsage();
                    exit(0);
                }
                break;
            case 't':
                if (setDelay(optarg, &delay) == -1) {
                    printUsage();
                    exit(0);
                }
                break;
            case 'd':
                tmp = optarg;
                argd = true;
                break;
            case ':': {
                printf("l'opzione '-%c' richiede un argomento\n", optopt);
                printUsage();
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

    //pid=fork();
    pid = 1; //per testing vado solo su master
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {  //Gestione collector

    } else {  //Gestione Master

        Master(&q, argv, qlen, nthread, argd, argc, tmp);
        char * var = NULL;
        //devo creare la socket (AF_UNIX) per comunicazione con il processo collector

        //devo creare la threadpool di n thread
        threadpool_t *pool = NULL;
        pool = createThreadPool(nthread, nthread);

        volatile long termina = 0;
        while (!termina) {
            char *args = pop(q);
            int r = addToThreadPool(pool, worker, (void *) args);
            if (r == 0) continue; // aggiunto con successo
            if (r < 0) { // errore interno
                fprintf(stderr, "FATAL ERROR, adding to the thread pool\n");
            } else { // coda dei pendenti piena
                fprintf(stderr, "SERVER TOO BUSY\n");
            }
            free(args);
        }
        destroyThreadPool(pool, 0);
    }
    deleteQueue(q, NULL);
    //ricordati di deallocare per i thread creati


    return 0;
}

void *Consumer(void *arg) {
    int myid = ((threadArgs_t *) arg)->thid;
    while (1) {
        char *data = malloc(sizeof(char) * 2000);
        data = pop(q);
        if (data == EOS) {
            break;
        }

        FILE *fp;
        //il worker dovra accedere al file e analizzarlo
        if ((fp = fopen(data, "rb")) == NULL) {
            perror("apertura file binario");
        }
        printf("Consumer%d: -- %s\n", myid, data);
        //fread()
        free(data);
    }

    printf("Consumer%d exits\n", myid);
    pthread_exit(NULL);
}



int setDelay(char *optArg, int *n) {
    int tmp;
    if (isNumber(optArg, &tmp) != 0) {
        printf("l'argomento di '-n' non e' valido\n");
        return -1;
    }
    printf("-n : %d\n", tmp);
    *n = tmp;
    return 0;
}

void printUsage() {
    printf("usage :./<name_program> [-n] <value> [-q] <value> [-t] <value> [-d] <pathname_to_dir>\n");
    fflush(stdout);
}

int setNThread(const char *optArg, int *n) {
    int tmp;
    if (isNumber(optArg, &tmp) != 0) {
        printf("l'argomento di '-n' non e' valido\n");
        return -1;
    }
    printf("-n : %d\n", tmp);
    *n = tmp;
    return 0;
}

int setQlen(const char *optArg, int *n) {
    int tmp;
    if (isNumber(optArg, &tmp) != 0) {
        printf("l'argomento di '-q' non e' valido\n");
        return -1;
    }
    printf("-q : %d\n", tmp);
    *n = tmp;
    return 0;
}

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




