#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <assert.h>
#include <sys/select.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include <bits/types/sig_atomic_t.h>
#include "includes/Queue.h"
#include "includes/Collector.h"
#include "includes/Master.h"
#include "includes/Conn.h"

#define N 100
#define EOS (void*)0x1
#define MAX_LENGHT_PATH 255

extern pthread_mutex_t lock;
extern int fc_skt;

int SendMsgToCollector(char *args, long result);

// funzione eseguita dal Worker thread
void * worker(void *queue) {
    Queue *q = (Queue *) queue;
    long result = 0;
    while (true) {
        char *args = pop(q);
        if (args == EOS) {
            return NULL;
        }
        else
        {
            FILE *f;
            long i = 0;
            long tmp;
             if ((f = fopen(args, "rb")) == NULL) {
                 printf("fallito [%s]", args);
                 perror("open file");
                 exit(EXIT_FAILURE);
             }
             while (fread(&tmp, sizeof(long), 1, f)) {
                 result = result + (i * tmp);
                 i++;
             }
             fclose(f);

             printf("sendin[%s]<--%ld on socket %d\n", args, result,fc_skt);
             fflush(stdout);
            }

            result = SendMsgToCollector(args,result);
        }
}

int SendMsgToCollector(char *args, long result){

    int len=strlen(args);
    int n;
    int tmp;
    int risposta;


    if((n=writen(fc_skt,&result,sizeof(long)))==-1){
        perror("writen result");
        int errno_copy = errno;
        exit(errno_copy);
    }

    if((n=writen(fc_skt,&len,sizeof(int)))==-1){
        perror("writen len file");
        int errno_copy = errno;
        exit(errno_copy);
    }
    if((n=writen(fc_skt,&args,sizeof(char)*len))==-1){
        perror("writen file");
        int errno_copy = errno;
        exit(errno_copy);
    }
    //vedo se il collector ha finito
    if((tmp=readn(fc_skt,&risposta,sizeof(int)))==-1){
        perror("read in sendCollector ");
        pthread_mutex_unlock(&lock);
        return 0;
    }

    close(fc_skt);

    return 0;
}