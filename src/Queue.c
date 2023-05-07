//
// Created by xab on 11/03/23.
//
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "includes/Queue.h"
extern volatile int finished_insert;
/**
 * @file boundedqueue.c
 * @brief File di implementazione dell'interfaccia per la coda di dimensione finita
 */


/* ------------------- funzioni di utilita' -------------------- */

// qui assumiamo (per semplicita') che le mutex non siano mai di
// tipo 'robust mutex' (pthread_mutex_lock(3)) per cui possono
// di fatto ritornare solo EINVAL se la mutex non e' stata inizializzata.

static inline void LockQueue(Queue *q) {
    if (pthread_mutex_lock(&q->m) != 0) {
        fprintf(stderr, "ERRORE FATALE lock\n");
        pthread_exit((void *) EXIT_FAILURE);
    }
}

static inline void UnlockQueue(Queue *q) {
    if (pthread_mutex_unlock(&q->m) != 0) {
        fprintf(stderr, "ERRORE FATALE unlock\n");
        pthread_exit((void *) EXIT_FAILURE);
    }
}

static inline void WaitToProduce(Queue *q) {
    if (pthread_cond_wait(&q->cfull, &q->m) != 0) {
        fprintf(stderr, "ERRORE FATALE wait\n");
        pthread_exit((void *) EXIT_FAILURE);
    }
}

static inline void WaitToConsume(Queue *q) {
    if (pthread_cond_wait(&q->cempty, &q->m) != 0) {
        fprintf(stderr, "ERRORE FATALE wait\n");
        pthread_exit((void *) EXIT_FAILURE);
    }
}

static inline void SignalProducer(Queue *q) {
    if (pthread_cond_signal(&q->cfull) != 0) {
        fprintf(stderr, "ERRORE FATALE signal\n");
        pthread_exit((void *) EXIT_FAILURE);
    }
}

static inline void SignalConsumer(Queue *q) {
    if (pthread_cond_signal(&q->cempty) != 0) {
        fprintf(stderr, "ERRORE FATALE signal\n");
        pthread_exit((void *) EXIT_FAILURE);
    }
}

/* ------------------- interfaccia della coda ------------------ */

Queue *initQueue(size_t n) {
    Queue *q = (Queue *) calloc(sizeof(Queue), 1);
    if (!q) {
        perror("malloc");
        return NULL;
    }
    q->buf = calloc(sizeof(void *), n);
    memset(q->buf, 0, n);
    if (!q->buf) {
        perror("malloc buf");
        free(q);
        return NULL;
    }
    if (pthread_mutex_init(&q->m, NULL) != 0) {
        perror("pthread_mutex_init");
        if (q->buf) free(q->buf);
        free(q);
        return NULL;
    }
    if (pthread_cond_init(&q->cfull, NULL) != 0) {
        perror("pthread_cond_init full");
        if (q->buf) free(q->buf);
        if (&q->m) pthread_mutex_destroy(&q->m);
        if (&q->cfull) pthread_cond_destroy(&q->cfull);
        if (&q->cempty) pthread_cond_destroy(&q->cempty);
        free(q);
        return NULL;
    }
    if (pthread_cond_init(&q->cempty, NULL) != 0) {
        perror("pthread_cond_init empty");
        return NULL;
    }
    q->head = q->tail = 0;
    q->qlen = 0;
    q->qsize = n;
    return q;
}

void deleteQueue(Queue *q, void (*F)(void *)) {
    if (!q) {
        errno = EINVAL;
        return;
    }
    if (F) {
        void *data = NULL;
        while ((data = pop(q))) F(data);
    }
    if (q->buf) free(q->buf);
    if (&q->m) pthread_mutex_destroy(&q->m);
    if (&q->cfull) pthread_cond_destroy(&q->cfull);
    if (&q->cempty) pthread_cond_destroy(&q->cempty);
    free(q);
}

int push(Queue *q, void *data) {
    if (!q || !data) {
        errno = EINVAL;
        return -1;
    }
    LockQueue(q);
    while (q->qlen == q->qsize) WaitToProduce(q);
    assert(q->buf[q->tail] == NULL);
    q->buf[q->tail] = data;
    q->tail += (q->tail + 1 >= q->qsize) ? (1 - q->qsize) : 1;
    q->qlen += 1;
    /* Invece di fare sempre la signal, si puo' contare il n. di
     * consumer in attesa e fare la signal solo se tale numero
     * e' > 0
     */
    SignalConsumer(q);
    UnlockQueue(q);
    return 0;
}

void *pop(Queue *q) {
    if (!q) {
        errno = EINVAL;
        return NULL;
    }
    LockQueue(q);
    while (q->qlen == 0) {
        //if(finished_insert == 1) return (void*)0x1;
        WaitToConsume(q);
    }
    void *data = q->buf[q->head];
    q->buf[q->head] = NULL;
    q->head += (q->head + 1 >= q->qsize) ? (1 - q->qsize) : 1;
    q->qlen -= 1;
    assert(q->qlen >= 0);
    /* Invece di fare sempre la signal, si puo' contare il n. di
     * producer in attesa e fare la signal solo se tale numero
     * e' > 0
     */
    SignalProducer(q);
    UnlockQueue(q);
    return data;
}

bool QueueIsEmpty(Queue *q){
    bool res = false;
    LockQueue(q);
    if(q->qlen==0) res = true;
    UnlockQueue(q);
    return res;
}