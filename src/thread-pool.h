#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <glib.h>

typedef void (*ThreadPoolFun)(GAsyncQueue *output, gpointer task);

typedef struct _ThreadPool {
    guint size;
    pthread_t *threads;
    GAsyncQueue *input_queue;
    GAsyncQueue *output_queue;
    gboolean exit;
    ThreadPoolFun worker;
} ThreadPool;

ThreadPool *thread_pool_new        (guint size, ThreadPoolFun worker);
void        thread_pool_destroy    (ThreadPool *tp);
void        thread_pool_push       (ThreadPool *tp, gpointer task);
gpointer    thread_pool_pop        (ThreadPool *tp);
guint       thread_pool_size       (ThreadPool *tp);

#endif
