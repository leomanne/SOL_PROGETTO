//
// Created by xab on 11/03/23.
//

#if !defined(QUEUE_H)
#define QUEUE_H

#include <pthread.h>

/** Struttura dati coda.
 *
 */
typedef struct Queue {
    void **buf;
    size_t head;
    size_t tail;
    size_t qsize;
    size_t qlen;
    pthread_mutex_t m;
    pthread_cond_t cfull;
    pthread_cond_t cempty;
} Queue;


/** Alloca ed inizializza una coda di dimensione \param n.
 *  Deve essere chiamata da un solo thread (tipicamente il thread main).
 *
 *   \retval NULL se si sono verificati problemi nell'allocazione (errno settato)
 *   \retval q puntatore alla coda allocata
 */
Queue *initQueue(size_t n);

/** Cancella una coda allocata con initQueue. Deve essere chiamata da
 *  da un solo thread (tipicamente il thread main).
 *
 *   \param q puntatore alla coda da cancellare
 */
void deleteQueue(Queue *q, void (*F)(void *));

/** Inserisce un dato nella coda.
 *   \param data puntatore al dato da inserire
 *
 *   \retval 0 se successo
 *   \retval -1 se errore (errno settato opportunamente)
 */
int push(Queue *q, void *data);

/** Estrae un dato dalla coda.
 *
 *  \retval data puntatore al dato estratto.
 */
void *pop(Queue *q);

#endif /* QUEUE_H */