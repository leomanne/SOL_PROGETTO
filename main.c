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
#define NTHREAD 4
#define QLEN 8
#define DELAY 0
#define MAX_LENGHT_PATH 255
#define EXIT_MSG "-exit-"
#define EOS (void*)0x1
Queue *q;
typedef struct threadArgs {
    int      thid;
    Queue *q;
} threadArgs_t;

int isNumber(const char* s, int* n);
int setNThread(const char* m, int *n);
int setQlen(const char* m, int *n);
int isdot(const char dir[]);
void lsR(const char nomedir[]);
void printUsage();
int checkCommand(char **pString, int i);

int setDelay(char *optarg, int *pInt);
void *Consumer(void *arg);
int CheckDir(char *optarg);

int checkCommand(char **pString, int i);

int CheckFile(char *string);

int main(int argc, char *argv[]) {
    int nthread = NTHREAD;
    int status ; /* conterra’ lo stato */
    int qlen = QLEN;
    int pid;
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

        q = initQueue(qlen);//coda per gli elementi da mandare ai thread workers
        if (!q) {
            fprintf(stderr, "initBQueue fallita\n");
            exit(errno);
        }
        if (argd) { //fai il salvataggio dei path usando la dir passata con -d
            if (CheckDir(tmp) == 1) {
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
                    if (CheckFile(argv[i]) == 1) {
                        printUsage();
                        return EXIT_FAILURE;
                    }
                }
            }
        }
        pid=fork();
        if(pid==-1){
            perror("fork");
            exit(EXIT_FAILURE);
        }
        else if(pid==0){  //Gestione collector

        }else{  //Gestione Master



        //devo creare la socket (AF_UNIX) per comunicazione con il processo collector

        //devo creare la threadpool di n thread

        //ricordati di deallocare per i thread creati
    }

        deleteQueue(q, NULL);

    return 0;
}
void *Consumer(void *arg) {
    int   myid  = ((threadArgs_t*)arg)->thid;
    while(1) {
        char *data = malloc(sizeof(char)*2000);
        data = pop(q);
        if (data == EOS){
            break;
        }

        FILE *fp;
        //il worker dovra accedere al file e analizzarlo
        if((fp = fopen(data,"rb"))==NULL){
            perror("apertura file binario");
        }
        printf("Consumer%d: -- %s\n",myid,data);
        //fread()
        free(data);
    }

    printf("Consumer%d exits\n",myid);
    pthread_exit(NULL);
}
int CheckFile( char *string) {

    struct stat statbuf;
    int r;
    if ((r=stat(string,&statbuf)) == -1){
        //NEL CASO IMPLEMENTA DA IL CONTROLLO DA ROOT "/" fino a trovare la cartella interessata
        perror("Facendo stat del file");
        return EXIT_FAILURE;
    }
    if(S_ISREG(statbuf.st_mode)){
        printf("push normal file [%s]\n",string);

            char *data = malloc(sizeof(char)* strlen(string)+1);
            if (data == NULL) {
                perror("Producer malloc");
                pthread_exit(NULL);
            }
            strncpy(data,string, strlen(string));
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
    if(S_ISDIR(statbuf.st_mode)) {
        //printf("entro in lsr\n");
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
                fprintf(stderr, "ERRORE: MAX_LENGHT_PATH troppo piccolo\n");
                exit(EXIT_FAILURE);
            }
            strncpy(filename,nomedir,      MAX_LENGHT_PATH-1);
            //printf("---->%c<----\n",filename[strlen(filename)-1]);
            if(filename[strlen(filename)-1]!='/') {
                strncat(filename, "/", MAX_LENGHT_PATH - 1);
            }
            strncat(filename,file->d_name, MAX_LENGHT_PATH-1);

            if (stat(filename, &statbuf)==-1) {
                perror("eseguendo la stat");
                return;
            }
            if(S_ISDIR(statbuf.st_mode)) {
                if ( !isdot(filename) ) lsR(filename);
            } else {
                //strncpy(data,filename, strlen(data)-1);
                if(CheckFile(filename)==0){
                    printf("ok check file ha fatto push\n");
                }else{
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




/*if (( pid = fork() ) == -1) {
        perror("main: fork"); exit(EXIT_FAILURE);
    }
/*if ( pid!=0 ) {*//* figlio COLLECTOR
printf("Processo %d, figlio.\n",getpid());
/* termina con stato 17 */
/*}else { *//* padre MASTERWORKER */

/*pid = waitpid(pid, &status, 0);
if (WIFEXITED(status)) {*//* il figlio terminato con exit o return */
/*    printf("stato %d\n", WEXITSTATUS(status));
}


printf("[%zu]\n", q->qlen);
pthread_t *th;
threadArgs_t *thARGS;
int p = nthread / 2;
int c = nthread / 2;
th = malloc((p + c) * sizeof(pthread_t));
thARGS = malloc((p + c) * sizeof(threadArgs_t));
if (!th || !thARGS) {
fprintf(stderr, "malloc fallita\n");
exit(EXIT_FAILURE);
}
if (!q) {
fprintf(stderr, "initBQueue fallita\n");
exit(errno);
}
for (int i = 0; i < p; ++i) {
thARGS[i].thid = i;
thARGS[i].q = q;
}
for (int i = p; i < (p + c); ++i) {
thARGS[i].thid = i - p;
thARGS[i].q = q;
}
for (int i = 0; i < c; ++i)
if (pthread_create(&th[p + i], NULL, Consumer, &thARGS[p + i]) != 0) {
fprintf(stderr, "pthread_create failed (Consumer)\n");
exit(EXIT_FAILURE);
}



// produco tanti EOS quanti sono i consumatori
for (int i = 0; i < c; ++i) {
push(q, EOS);
}
// aspetto la terminazione di tutti i consumatori
for (int i = 0; i < c; ++i)
pthread_join(th[p + i], NULL);

// libero memoria
free(th);
free(thARGS);

/*}*/