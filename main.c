#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include "includes/Queue.h"
//MasterWorker
#define NTHREAD 4
#define QLEN 8
#define DELAY 0
#define MAX_LENGHT_PATH 255



int isNumber(const char* s, int* n);
int setNThread(const char* m, int *n);
int setQlen(const char* m, int *n);
int isdot(const char dir[]);
void lsR(const char nomedir[]);
void printUsage();
int checkCommand(char **pString, int i);

int setDelay(char *optarg, int *pInt);

int CheckDir(char *optarg);

int checkCommand(char **pString, int i);

int main(int argc, char *argv[]) {
    int nthread = NTHREAD;
    int qlen = QLEN;
    int delay = DELAY;
    //bool argd = false;
    //char *nameDir;
    if (argc==1) {
        printf("almeno una opzione deve essere passata\n");
        return -1;
    }

    int opt;
    /*Queue *q = (Queue *)calloc(sizeof(Queue), 1);
    if (!q) { perror("malloc"); return 1;}
    q->nomi = calloc(sizeof(void*), n);
    if (!q->buf) {
        perror("malloc buf");
        goto error;
    }
    if (pthread_mutex_init(&q->m,NULL) != 0) {
        perror("pthread_mutex_init");
        goto error;
    }
    if (pthread_cond_init(&q->cfull,NULL) != 0) {
        perror("pthread_cond_init full");
        goto error;
    }
    if (pthread_cond_init(&q->cempty,NULL) != 0) {
        perror("pthread_cond_init empty");
        goto error;
    }
    q->head  = q->tail = 0;
    q->qlen  = 0;
    q->qsize = n;
    return q;*/


    while ((opt = getopt(argc,argv, ":n:q:t:d:")) != -1) {
        switch(opt) {
            case 'n':
                if(setNThread(optarg,&nthread)==-1){
                    printUsage();
                    return EXIT_FAILURE;
                }
                printf("nthreads = [%d]",nthread);  break;
            case 'q':
                if(setQlen(optarg,&qlen)==-1){
                    printUsage();
                    return EXIT_FAILURE;
                }  break;
            case 't': if(setDelay(optarg,&delay)==-1){
                    printUsage();
                    return EXIT_FAILURE;
                } break;
            case 'd': if(CheckDir(optarg)==1) {
                    printUsage();
                    return EXIT_FAILURE;
                }else{
                    printf("%s e' una directory\n",optarg);

            }break;
            case ':': {
                printf("l'opzione '-%c' richiede un argomento\n", optopt);
                printUsage();
            } break;
            case '?': {  // restituito se getopt trova una opzione non riconosciuta
                printf("l'opzione '-%c' non e' gestita\n", optopt);
                printUsage();

            } break;
            default:;
        }
    }

    for (int i = 0; i < argc; ++i) {
        if(argv[i]!=NULL){
            if(checkCommand(argv,i)==0){//se e' = 0 allora prendiamo l'elemento successivo del successivo
                i++;
            }else{
                //ci salviamo questo elemento perche dobbiamo controllare se e' un file.dat da mandare ai workers

            }
        }


    }
    return 0;
}

int checkCommand(char **argv, int i) {
    return strncmp(argv[i],"-",1);
}

int CheckDir(char *optarg){

    const char *dir  = optarg;

    struct stat statbuf;
    int r;
    if ((r=stat(dir,&statbuf)) == -1){
        //NEL CASO IMPLEMENTA DA IL CONTROLLO DA ROOT "/" fino a trovare la cartella interessata
	    perror("Facendo stat del file");
        return EXIT_FAILURE;
    }
    if(!S_ISDIR(statbuf.st_mode)) {
        //se non e' una directory
        fprintf(stderr, "%s non e' una directory\n", dir);
        //mi salvo il path assoluto

        return EXIT_FAILURE;
    }
    if(S_ISDIR(statbuf.st_mode)) {
       //se e' una directory allora salvati ricorsivamente tutti i file .dat da qualche parte
        lsR(optarg);
    }
    return -1;
}
int isdot(const char dir[]) {
    int l = strlen(dir);

    if ( (l>0 && dir[l-1] == '.') ) return 1;
    return 0;
}
void lsR(const char nomedir[]) {
    // controllo che il parametro sia una directory
    struct stat statbuf;
    int r;

    if ((r=stat(nomedir,&statbuf)) == -1) {
        perror("Facendo stat del file");
        return ;
    }
  ;
    DIR * dir;
    fprintf(stdout, "-----------------------\n");
    fprintf(stdout, "Directory %s:\n",nomedir);

    if ((dir=opendir(nomedir)) == NULL) {
        perror("opendir");
        return;
    } else {
        struct dirent *file;

        while((errno=0, file =readdir(dir)) != NULL) {
            struct stat statbuf;
            char filename[MAX_LENGHT_PATH];
            int len1 = strlen(nomedir);
            int len2 = strlen(file->d_name);
            if ((len1+len2+2)>MAX_LENGHT_PATH) {
                fprintf(stderr, "ERRORE: MAXFILENAME troppo piccolo\n");
                exit(EXIT_FAILURE);
            }
            strncpy(filename,nomedir,      MAX_LENGHT_PATH-1);
            strncat(filename,"/",          MAX_LENGHT_PATH-1);
            strncat(filename,file->d_name, MAX_LENGHT_PATH-1);

            if (stat(filename, &statbuf)==-1) {
                perror("eseguendo la stat");
                return;
            }
            if(S_ISDIR(statbuf.st_mode)) {
                if ( !isdot(filename) ) lsR(filename);
            } else {


                //se voglio il path assoluto uso filename non file->d_name
                fprintf(stdout, "%20s: %10ld \n", file->d_name, statbuf.st_size);
            }
        }
        if (errno != 0) perror("readdir");
        closedir(dir);
        fprintf(stdout, "-----------------------\n");
    }
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

int setNThread(const char* optArg,int *n) {
    int tmp;
    if (isNumber(optArg, &tmp) != 0) {
        printf("l'argomento di '-n' non e' valido\n");
        return -1;
    }
    printf("-n : %d\n", tmp);
    *n = tmp;
    return 0;
}
int setQlen(const char* optArg,int *n) {
    int tmp;
    if (isNumber(optArg, &tmp) != 0) {
        printf("l'argomento di '-q' non e' valido\n");
        return -1;
    }
    printf("-q : %d\n", tmp);
    *n = tmp;
    return 0;
}

int isNumber(const char* s, int* n) {
    if (s==NULL) return 1;
    if (strlen(s)==0) return 1;
    char* e = NULL;
    errno=0;
    long val = strtol(s, &e, 10);
    if (errno == ERANGE) return 2;    // overflow
    if (e != NULL && *e == (char)0) {
        *n = val;
        return 0;   // successo
    }
    return 1;   // non e' un numero
}


