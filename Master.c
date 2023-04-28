//
// Created by xab on 03/04/23.
//
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include <bits/types/sig_atomic_t.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <unistd.h>
#include "includes/Queue.h"
#include <sys/un.h>



#define MAX_LENGHT_PATH 255
#define UNIX_PATH_MAX 108
#define SOCKETNAME "./socket"
volatile sig_atomic_t quit=0; //usato per gestire uscite
extern volatile int finished_insert;
int CreaSocket();
int checkCommand(char **pString, int i);
void *Insert(void *info);
void recursiveInsert(const char nomedir[],Queue *q);

int CheckFile(char *string,Queue *q);

int isdot(const char dir[]);

int CheckDir(char *optarg,Queue *q);

void printUsage();

int CreaSocket(){
    //preparo il socket address
    int fc_skt;
    struct sockaddr_un sa;
    strncpy(sa.sun_path, SOCKETNAME,UNIX_PATH_MAX);
    sa.sun_family=AF_UNIX; //setto ad AF_UNIX
    //creo la socket
    fc_skt=socket(AF_UNIX,SOCK_STREAM,0);
    errno=0;
    //ciclo finche non riesco a connettermi
    while((connect(fc_skt, (struct sockaddr*)&sa, sizeof(sa)))==-1){
        if(errno>0){
            sleep(1);
        }
    }

    return fc_skt;
}

void * Insert(void *info){
    int   argc  = ((infoInsert *)info)->argc;
    bool   argd  = ((infoInsert *)info)->argd;
    char*  tmp  = ((infoInsert *)info)->tmp;
    char ** argv = ((infoInsert *)info)->argv;
    Queue **q =  ((infoInsert *) info)->q;
    int nthreads = ((infoInsert *)info)->nthreads;
    if (argd) { //fai il salvataggio dei path usando la dir passata con -d
        if (CheckDir(tmp,*q) == 1) {
            printUsage();
            return NULL;
        } else {
            printf("%s e' una directory\n", tmp);
        }
    }
    for (int i = 1; i < argc; ++i) {
        if (argv[i] != NULL) {
            if (checkCommand(argv, i) == 0) {//se e' = 0 allora prendiamo l'elemento successivo del successivo
                i++;
            } else {
                //ci salviamo questo elemento perche dobbiamo controllare se e' un file.dat da mandare ai workers
                if (CheckFile(argv[i],*q) == 0) {

                } else {
                    printUsage();
                    return NULL;
                }
            }
        }
    }
    finished_insert = 1;

    //inserisco i messaggi in coda al termine per segnalare fine della inserzione
    for (int i = 0 ; i < nthreads; ++i) {
        push(*q,(void*)0x1);
    }
    return NULL;
}

int CheckDir(char *optarg,Queue *q) {

    const char *dir = optarg;
    struct stat statbuf;
    int r;
    if ((r = stat(dir, &statbuf)) == -1) {
        //NEL CASO IMPLEMENTA DA IL CONTROLLO DA ROOT "/" fino a trovare la cartella interessata
        perror("Facendo stat del file");
        return EXIT_FAILURE;
    }
    if(S_ISDIR(statbuf.st_mode)) {
        //printf("entro in lsr\n");
        recursiveInsert(optarg,q);
    }else{
        return 1; //segnalo che non era una dir ma un file normale
    }

    return -1;
}
void recursiveInsert(const char nomedir[],Queue *q) {
    // controllo che il parametro sia una directory
    struct stat statbuf;
    int r;
    if ((r = stat(nomedir, &statbuf)) == -1) {
        perror("Facendo stat del file");
        printf("{%s}\n",nomedir);
        return;
    }
    DIR *dir;
    //fprintf(stdout, "-----------------------\n");
    //fprintf(stdout, "Directory %s:\n", nomedir);
    if ((dir = opendir(nomedir)) == NULL) {
        perror("opendir Error");
        return;
    } else {
        struct dirent *file;
        while ((errno = 0, file = readdir(dir)) != NULL) {
            char filename[MAX_LENGHT_PATH];
            int len1 = strlen(nomedir);
            int len2 = strlen(file->d_name);

            if ((len1 + len2 + 2) > MAX_LENGHT_PATH) {
                fprintf(stderr, "ERRORE: MAX_LENGHT_PATH troppo piccolo\n");
                exit(EXIT_FAILURE);
            }
            strncpy(filename, nomedir, MAX_LENGHT_PATH - 1);
            //printf("---->%c<----\n",filename[strlen(filename)-1]);
            if (filename[strlen(filename) - 1] != '/') {
                strncat(filename, "/", MAX_LENGHT_PATH - 1);
            }
            strncat(filename, file->d_name, MAX_LENGHT_PATH - 1);

            if (stat(filename, &statbuf) == -1) {
                perror("eseguendo la stat");
                return;
            }
            if (S_ISDIR(statbuf.st_mode)) {
                if (!isdot(filename)) recursiveInsert(filename,q);
            } else {
                //strncpy(data,filename, strlen(data)-1);
                if (CheckFile(filename,q) == 0) {
                    //printf("ok check file ha fatto push\n");
                } else {
                    //printf("ok check file non ha fatto push\n");
                }
                //se voglio il path assoluto uso filename non file->d_name
                //fprintf(stdout, "%20s: %10ld \n", file->d_name, statbuf.st_size);
            }

        }
        if (errno != 0) perror("readdir");
        closedir(dir);
        //fprintf(stdout, "------------------------\n");
    }

}

int isdot(const char dir[]) {
    int l = strlen(dir);
    if ((l > 0 && dir[l - 1] == '.')) return 1;
    return 0;
}

int CheckFile(char *string,Queue *q) {

    struct stat statbuf;
    int r;
    if ((r = stat(string, &statbuf)) == -1) {
        //NEL CASO IMPLEMENTA DA IL CONTROLLO DA ROOT "/" fino a trovare la cartella interessata
        perror("Facendo stat del file");
        return EXIT_FAILURE;
    }
    if (S_ISREG(statbuf.st_mode)) {
        //printf("push normal file [%s]\n", string);

        char *data = malloc(sizeof(char) * strlen(string)+1);
        if (data == NULL) {
            perror("Producer malloc");
            pthread_exit(NULL);
        }
        data = memcpy(data, string, strlen(string)+1);
        data[strlen(string)]= '\0';
        if (push(q, data) == -1) {
            fprintf(stderr, "Errore: push\n");
        }
        return 0;
    }
    return -1;
}

int checkCommand(char **argv, int i) {
    return strncmp(argv[i], "-", 1);
}