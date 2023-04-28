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
#define EOS (void*)0x1
#define MAX_LENGHT_PATH 255
extern volatile int finished_insert;
// funzione eseguita dal Worker thread del pool
static int count = 0;
void * worker(void *queue) {
    Queue *q = (Queue *) queue;
    //bisogna mettere un while con un controllo per FINITO

    while (true) {
        char *args = pop(q);
        //printf("[%s]\n",args);
        if (args == EOS) {
            count++;
            printf("count -> [%d]\n", count);
            fflush(stdout);
            return NULL;
        } else {
            FILE *f;
            long result = 0;
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

             printf("[%s]<--%ld\n", args, result);
             fflush(stdout);
            }
        }
}