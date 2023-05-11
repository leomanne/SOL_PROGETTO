#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <unistd.h>
#include "includes/Queue.h"
#include "includes/Conn.h"
#include <sys/un.h>

//----------------------------------------------------------

#define MAX_LENGHT_PATH 255

//----------------------------------------------------------

extern pthread_mutex_t lock;


//----------------------------------------------------------

int fc_skt;
volatile sig_atomic_t  sig = 0;
volatile sig_atomic_t usr1=0;
struct sigaction s;

//----------------------------------------------------------

int CreaSocketClient();
int checkCommand(char **argv, int i);
void *Insert(void *info, int fcSkt);
void recursiveInsert(const char* nomedir,Queue *q,int delay);
int AddFileToQueue(char *string,Queue *q,int delay);
int isdot(const char dir[]);
void printUsage();
int TellCollectorPrint(int skt);
int AddHandler();
static void sighandler(int signal);
static void sighandler_usr1(int signal);

//----------------------------------------------------------


/**
 * Questo metodo crea un socket e tenta di connettersi con il collector su "./cs_sock" definito il Conn.h
 * @return -1 se un errore e' stato rilevato , il valore
 */
int CreaSocketClient(){
    //preparo il socket address
    struct sockaddr_un sa;
    strncpy(sa.sun_path, SOCKNAME,strlen(SOCKNAME)+1);
    sa.sun_family=AF_UNIX; //setto ad AF_UNIX
    //creo la socket
    fc_skt=socket(AF_UNIX,SOCK_STREAM,0);
    if(fc_skt==-1){
        perror("socket server");
        return -1;
    }
    errno=0;
    //ciclo finche non riesco a connettermi
    while((connect(fc_skt, (struct sockaddr*)&sa, sizeof(sa)))==-1){
        if(errno>0){
            sleep(1);
        }
    }
    return fc_skt; //ritorno il file descriptor
}
/**
 * Questa funzione inserisce i file passati nella coda concorrente.
 * @param info struttura dati contenente le informazioni dei flag e argomenti passati, compresa la coda concorrente
 * @return NULL
 */
void *Insert(void *info, int fcSkt) {
    int   argc  = ((infoInsert *)info)->argc;
    int   delay  = ((infoInsert *)info)->delay;
    bool   argd  = ((infoInsert *)info)->argd;
    char*  tmp  = ((infoInsert *)info)->tmp;
    char ** argv = ((infoInsert *)info)->argv;
    Queue **q =  ((infoInsert *) info)->q;
    int nthreads = ((infoInsert *)info)->nthreads;


    if (argd) { //fai il salvataggio dei path usando la dir passata con -d se il flag e' stato settato
        recursiveInsert(tmp,*q,delay);//inserisci ricorsivamente tutti i file in tmp nella coda q con un delay.
    }

    //controllo ogni valore passato da riga di comando
    for (int i = 1; i < argc; ++i) {
        if(sig==1){
            break;
        }
        if(usr1==1){
            int x = TellCollectorPrint(fcSkt);
            if(x == -1) break;//termina senza continuare l'inserimento
            usr1 = 0;
        }
        if (argv[i] != NULL) {
            if (checkCommand(argv, i) == 0) {//se e' = 0 allora saltiamo il prossimo elemento
                i++;
            } else {
                //ci salviamo questo elemento perche dobbiamo controllare se e' un file.dat da mandare ai workers
                if (AddFileToQueue(argv[i],*q,delay) == 0) {
                } else {
                    printf("Errore file %s e' una dir\n",argv[i]);
                }
            }
        }
    }
    //inserisco i messaggi in coda al termine per segnalare fine della insersione. una push per ogni thread
    for (int i = 0 ; i < nthreads; ++i) {
        push(*q,(void*)0x1);
    }
    //printf("\nFINE INSERT\n");
    fflush(stdout);
    return NULL;

}
/**
 * Funzione ricorsiva che controlla tutti i file passati attraverso -d. in caso di SIGNAL USR1 dice al proc collector di stampare cio che ha ricevuto
 * @param nomedir stringa contenente il file da controllare (puo essere file regolare oppure dir)
 * @param q coda concorrente
 * @param delay tempo in intero su cui usare la funzione sleep (attenzione a convertire il millisecondi)
 */
void recursiveInsert(const char* nomedir,Queue *q,int delay) {
    // controllo che il parametro sia una directory
    if(sig==1)return;
    if(usr1==1){
        int x = TellCollectorPrint(fc_skt);
        if(x == -1) return ;//termina senza continuare l'inserimento se si e' verificato qualche errore
        usr1 = 0;
    }
    struct stat statbuf;
    int r;
    if ((r = stat(nomedir, &statbuf)) == -1) {
        perror("Facendo stat del file");
        printf("{%s}\n",nomedir);
        return;
    }
    if(S_ISDIR(statbuf.st_mode)) {//se il file e' una directory controllo tutti i file e ricorsivamente li salvo nella coda concorrente
        DIR *dir;
        if ((dir = opendir(nomedir)) == NULL) {
            perror("opendir Error");
            return;
        } else {
            struct dirent *file;
            while ((errno = 0, file = readdir(dir)) != NULL) { //ciclo su tutti i file della directory dir
                char filename[MAX_LENGHT_PATH];
                int len1 = strlen(nomedir);
                int len2 = strlen(file->d_name);
                //controllo se la lunghezza totale supera il MAX_LENGHT_PATH (255)
                if ((len1 + len2 + 2) > MAX_LENGHT_PATH) {
                    fprintf(stderr, "ERRORE: MAX_LENGHT_PATH troppo piccolo\n");
                    exit(EXIT_FAILURE);
                }

                //copio in filename il nomedir+/+file->d_name
                strncpy(filename, nomedir, MAX_LENGHT_PATH - 1);
                if (filename[strlen(filename) - 1] != '/') {
                    strncat(filename, "/", MAX_LENGHT_PATH - 1);
                }
                strncat(filename, file->d_name, MAX_LENGHT_PATH - 1);

                if (stat(filename, &statbuf) == -1) {
                    perror("eseguendo la stat");
                    return;
                }
                if (S_ISDIR(statbuf.st_mode)) {//se filename e' una dir controllo che non sia "." o ".." e ricorsivamente controllo tutti i file contenuti in esso
                    if (!isdot(filename)) recursiveInsert(filename,q,delay);
                } else {//altrimenti faccio l'inserimento del file nella coda
                    if (AddFileToQueue(filename,q,delay) == -1) {
                        printf("Errore %s non inserito: non regular file\n",filename);
                    }
                }
            }
            if (errno != 0) perror("readdir");
            closedir(dir);
        }
    }
}
/**
 * Funzione che comunica al collector della ricezione del segnale USR1
 * @param skt file descriptor della socket
 * @return -1 se c'e' stato qualche errore,1 altrimenti
 */
int TellCollectorPrint(int skt) {
    long msg=-1;  //valore speciale per far capire al Collector che il MasterWorker vuole comunicargli SIGUSR1 signal
    int r;
    //uso il lock per il socket
    pthread_mutex_lock(&lock);
    if((r=writen(skt,&msg,sizeof(long)))==-1){
        perror("write");
        pthread_mutex_unlock(&lock);
        return -1;
    }
    if((r=readn(skt,&msg,sizeof(int)))==-1){
        perror("read");
        return -1;
    }
    if(r==0){
        pthread_mutex_unlock(&lock);//se la socket e' stata chiusa sblocco il lock
        return -1;//socket chiusa
    }
    pthread_mutex_unlock(&lock);
    return 1;
}

/**
 *
 * @param dir e' la stringa passata che corrisponde al nome del file da controllare
 * @return 1 se il file contiene "." come ultimo carattere, 0 altrimenti
 */
int isdot(const char* dir) {
    int l = strlen(dir);
    if ((l > 0 && dir[l - 1] == '.')) return 1;
    return 0;
}
/**
 *
 * @param string nome del file da inserire nella coda concorrente
 * @param q coda concorrente
 * @param delay valore di attesa usato tra un inserimento e il successivo
 * @return -1 se c'e' stato qualche errore, 0 altrimenti
 */
int AddFileToQueue(char *string,Queue *q,int delay) {
        char *data = malloc(sizeof(char) * strlen(string)+1);
        if (data == NULL) {
            perror("Producer malloc");
            return -1;
        }
        data = memcpy(data, string, strlen(string)+1);
        data[strlen(string)]= '\0';
        if(delay!=0){
            sleep(delay*0.001); //riduco il valore altrimenti interrompo per secondi non millisecondi
        }
        if (push(q, data) == -1) {
            fprintf(stderr, "Errore: push\n");
            return -1;
        }
    return 0;
}
/**
 *
 * @param argv array di stringhe passate da riga di comando
 * @param i indice per accedere al valore corretto in argv
 * @return 0 se il valore inizia con - e quindi e' un comando,!=0 altrimenti
 */
int checkCommand(char **argv, int i) {
    return strncmp(argv[i], "-", 1);
}
/**
 * Funzione utilizzata per installare i metodi usati per la gestione dei segnali
 * @return -1 se c'e' stato qualche errore, 0 altrimenti
 */
int AddHandler(){

    memset(&s,0,sizeof(s));
    s.sa_handler=sighandler; //uso sighandler per tutti i segnali specificati tranne sigusr1
    if(sigaction(SIGTERM,&s,NULL)==-1){
        perror("handler for SIGUSR1");
        return -1;
    }
    if(sigaction(SIGHUP,&s,NULL)==-1){
        perror("handler for SIGHUP");
        return -1;
    }
    if(sigaction(SIGINT,&s,NULL)==-1){
        perror("handler for SIGINT");
        return -1;
    }
    if(sigaction(SIGQUIT,&s,NULL)==-1){
        perror("handler for SIGQUIT");
        return -1;
    }
    s.sa_handler = sighandler_usr1;
    if(sigaction(SIGUSR1,&s,NULL)==-1){
        perror("handler for SIGUSR1");
        return -1;
    }
    return 0;
}
/**
 * setta il flag del segnale usr1
 * @param signal segnale catturato
 */
static void sighandler_usr1(int signal) {
        usr1 = 1;
}
/**
 * setta il flag utilizzato per tutti i segnali specificati tranne che sigusr1
 * @param signal segnale catturato
 */
static void sighandler(int signal) {
        sig = 1;
}
