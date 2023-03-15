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
#include "Queue.h"
//MasterWorker
#define NTHREAD 4
#define QLEN 8
#define DELAY 0
#define MAX_LENGHT_PATH 255



int isNumber(const char* s, int* n);
int setNThread(const char* m, int *n);
int setQlen(const char* m, int *n);
int isdot(const char dir[]);
void lsR(const char nomedir[],Queue *q);
void printUsage();
int checkCommand(char **pString, int i);

int setDelay(char *optarg, int *pInt);

int CheckDir(char *optarg,Queue *q);

int checkCommand(char **pString, int i);

int CheckFile(char *string, Queue *pQueue);

int main(int argc, char *argv[]) {
    int nthread = NTHREAD;
    int qlen = QLEN;
    int delay = DELAY;
    bool argd = false;
    //char *nameDir;
    if (argc==1) {
        printf("almeno una opzione deve essere passata\n");
        return -1;
    }

    int opt;


    char* tmp;
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
            case 'd':
                tmp = optarg;
                argd = true;
            break;
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
    Queue *q = initQueue(qlen);//coda per gli elementi da mandare ai thread workers
    if(argd){ //fai il salvataggio dei path usando la dir passata con -d
        if(CheckDir(tmp,q)==1) {
            printUsage();
            return EXIT_FAILURE;
        }else{
            printf("%s e' una directory\n",optarg);
        }
    }


    for (int i = 1; i < argc; ++i) {
        if(argv[i]!=NULL){
            if(checkCommand(argv,i)==0){//se e' = 0 allora prendiamo l'elemento successivo del successivo
                i++;
            }else{
                //ci salviamo questo elemento perche dobbiamo controllare se e' un file.dat da mandare ai workers
                if(CheckFile(argv[i],q)==1) {
                    printUsage();
                    return EXIT_FAILURE;
                }
            }
        }
    }
    printf("{%zu}\n",q->qlen);
    int len = q->qlen;
    for (int i = 0; i < len; ++i) {
        char *data;
        data = pop(q);
        printf("{{%s}}\n",data);
    }

    deleteQueue(q,NULL);
    return 0;
}

int CheckFile( char *string, Queue *q) {

    struct stat statbuf;
    int r;
    if ((r=stat(string,&statbuf)) == -1){
        //NEL CASO IMPLEMENTA DA IL CONTROLLO DA ROOT "/" fino a trovare la cartella interessata
        perror("Facendo stat del file");
        return EXIT_FAILURE;
    }
    if(S_ISREG(statbuf.st_mode)){
        push(q,string);
        return 0;
    }
    return -1;
}

int checkCommand(char **argv, int i) {
    return strncmp(argv[i],"-",1);
}

int CheckDir(char *optarg,Queue *q){

    const char *dir  = optarg;

    struct stat statbuf;
    int r;
    if ((r=stat(dir,&statbuf)) == -1){
        //NEL CASO IMPLEMENTA DA IL CONTROLLO DA ROOT "/" fino a trovare la cartella interessata
	    perror("Facendo stat del file");
        return EXIT_FAILURE;
    }
    if(S_ISDIR(statbuf.st_mode)) {
        lsR(optarg, q);
    }
    return -1;
}
int isdot(const char dir[]) {
    int l = strlen(dir);

    if ( (l>0 && dir[l-1] == '.') ) return 1;
    return 0;
}
void lsR(const char nomedir[],Queue *q) {
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
        perror("opendir Error");
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
                if ( !isdot(filename) ) lsR(filename,q);
            } else {
                char *data = malloc(sizeof(char)* strlen(filename));
                if (data == NULL) {
                    perror("Producer malloc");
                }
                if (push(q, data) == -1) {
                    perror("Push Error");
                }
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


