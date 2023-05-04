#if !defined(MASTER_)
#define MASTER_

#include <stdbool.h>
#include "Queue.h"

// funzione eseguita dal generico thread creato nel processo MASTER
void * Insert(void *info);
int CreaSocketServer();
#endif