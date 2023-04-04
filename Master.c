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
#include "includes/Queue.h"

#define MAX_LENGHT_PATH 255

int checkCommand(char **pString, int i);

void lsR(const char nomedir[],Queue *q);

int CheckFile(char *string,Queue *q);

int isdot(const char dir[]);

int CheckDir(char *optarg,Queue *q);

void printUsage();

int Master(Queue **q, char **argv, int qlen, int nthread, bool argd, int argc, char *tmp) {
    *q = initQueue(qlen);//coda per gli elementi da mandare ai thread workers
    if (!*q) {
        fprintf(stderr, "initBQueue fallita\n");
        exit(errno);
    }
    if (argd) { //fai il salvataggio dei path usando la dir passata con -d
        if (CheckDir(tmp,*q) == 1) {
            printUsage();
            return EXIT_FAILURE;
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
                    return EXIT_FAILURE;
                }
            }
        }
    }

    return 0;
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
    if (S_ISDIR(statbuf.st_mode)) {
        //printf("entro in lsr\n");
        lsR(optarg,q);
    }
    return -1;
}


void lsR(const char nomedir[],Queue *q) {
    // controllo che il parametro sia una directory
    struct stat statbuf;
    int r;

    if ((r = stat(nomedir, &statbuf)) == -1) {
        perror("Facendo stat del file");
        return;
    }

    DIR *dir;
    fprintf(stdout, "-----------------------\n");
    fprintf(stdout, "Directory %s:\n", nomedir);

    if ((dir = opendir(nomedir)) == NULL) {
        perror("opendir Error");
        return;
    } else {
        struct dirent *file;
        while ((errno = 0, file = readdir(dir)) != NULL) {
            struct stat statbuf;
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
                if (!isdot(filename)) lsR(filename,q);
            } else {
                //strncpy(data,filename, strlen(data)-1);
                if (CheckFile(filename,q) == 0) {
                    printf("ok check file ha fatto push\n");
                } else {
                    printf("ok check file non ha fatto push\n");
                }
                //se voglio il path assoluto uso filename non file->d_name
                //fprintf(stdout, "%20s: %10ld \n", file->d_name, statbuf.st_size);
            }

        }
        if (errno != 0) perror("readdir");
        closedir(dir);
        fprintf(stdout, "------------------------\n");
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
        printf("push normal file [%s]\n", string);

        char *data = malloc(sizeof(char) * strlen(string) + 1);
        if (data == NULL) {
            perror("Producer malloc");
            pthread_exit(NULL);
        }


        strncpy(data, string, strlen(string));
        //*data = 1;
        if (push(q, data) == -1) {
            fprintf(stderr, "Errore: push\n");
        }

        //if(data)free(data);
        //printf("(%s)\n",(char*)pop(q));
        return 0;
    }
    return -1;
}

int checkCommand(char **argv, int i) {
    return strncmp(argv[i], "-", 1);
}