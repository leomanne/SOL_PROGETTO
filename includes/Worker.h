#if !defined(WORKER_)
#define WORKER_

// funzione eseguita dal generico Worker del pool di thread
void * worker(void *arg);
int SendMsgToCollector(char *args, long result);
#endif