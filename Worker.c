#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <assert.h>
#include <sys/select.h>
#include <stdio.h>

// funzione eseguita dal Worker thread del pool
void worker(void *file) {
    char *args = (char *) file;
    FILE *f;
    long result = 0;
    long i = 0;
    long tmp;

    if ((f = fopen(args, "rb")) == NULL) {
        printf("fallito %s",args);
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