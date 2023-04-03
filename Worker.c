#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <assert.h>
#include <sys/select.h>
#include <stdio.h>

// funzione eseguita dal Worker thread del pool
//
void worker(void *arg) {
    assert(arg);
    char * args = (char*)arg;
    printf("[%s]",args);
    fflush(stdout);
}