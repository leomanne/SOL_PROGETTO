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
#include "includes/Queue.h"
#include "includes/Threadpool.h"
#include "includes/Worker.h"
#include "includes/Master.h"

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

    char *tmp = NULL;
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

        q = initQueue(qlen);//coda per gli elementi da mandare ai thread workers
        if (!q) {
            fprintf(stderr, "initBQueue fallita\n");
            exit(errno);
        }

        infoInsert info;
        info.tmp = tmp;
        info.nthread = nthread;
        info.argc = argc;
        info.argv = argv;
        info.argd = argd;
        info.q = &q;
        pthread_t *th = malloc(sizeof(pthread_t));
        if (!th) {
            fprintf(stderr, "malloc fallita\n");
            return EXIT_FAILURE;
        }
        if (pthread_create((pthread_t *) &th, NULL, Insert, &info) != 0) {
            fprintf(stderr, "pthread_create failed\n");
            exit(EXIT_FAILURE);
        }

        //devo creare la socket (AF_UNIX) per comunicazione con il processo collector

        //devo creare la threadpool di n thread
        threadpool_t *pool = NULL;
        pool = createThreadPool(nthread, nthread);

        volatile long termina = 0;
        while (nthread!=0) {
            char *args = malloc(sizeof(char)*255);
            args = pop(q);
            printf("passo al thread [%s]\n",args);
            int r = addToThreadPool(pool, worker, (void *) args);
            if (r == 0) continue; // aggiunto con successo
            if (r < 0) { // errore interno
                fprintf(stderr, "FATAL ERROR, adding to the thread pool\n");
            } else { // coda dei pendenti piena
                fprintf(stderr, "SERVER TOO BUSY\n");
            }
            free(args);
            nthread--;
        }

        if (pthread_join(*th, NULL) == -1) {
            fprintf(stderr, "pthread_join failed\n");
        }
        //printf("%s\n",(char *)pop(q));
        destroyThreadPool(pool, 0);
        deleteQueue(q, NULL);
        //ricordati di deallocare per i thread creati
        sleep(2);

        return 0;
    }
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



