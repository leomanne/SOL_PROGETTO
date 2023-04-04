#if !defined(MASTER_)
#define MASTER_

#include <stdbool.h>
#include "Queue.h"

// funzione eseguita dal generico Worker del pool di thread
int Master(Queue **q, char **argv, int qlen, int nthread, bool argd, int argc, char *tmp);

#endif