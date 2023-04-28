#if !defined(MASTER_)
#define MASTER_

#include <stdbool.h>
#include "Queue.h"

// funzione eseguita dal generico Worker del pool di thread
void * Insert();
int CreaSocket();
#endif