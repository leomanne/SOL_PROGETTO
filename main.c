#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>
//MasterWorker
#define NTHREAD 4
#define QLEN 8
#define DELAY 0

int isNumber(const char* s, int* n);
int setNThread(const char* m, int *n);
int setQlen(const char* m, int *n);

void printUsage();

int setDelay(char *optarg, int *pInt);

int main(int argc, char *argv[]) {
    int nthread = NTHREAD;
    int qlen = QLEN;
    int delay = DELAY;
    if (argc==1) {
        printf("almeno una opzione deve essere passata\n");
        return -1;
    }

    int opt;
    // se il primo carattere della optstring e' ':' allora getopt ritorna
    // ':' qualora non ci sia l'argomento per le opzioni che lo richiedono
    // se incontra una opzione (cioe' un argomento che inizia con '-') e tale
    // opzione non e' in optstring, allora getopt ritorna '?'
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
            case 'd':   break;
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


    return 0;
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


