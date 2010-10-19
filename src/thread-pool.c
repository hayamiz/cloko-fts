
#include "thread-pool.h"

void
thread_pool_push (ThreadPool *tp, gpointer task)
{
    g_async_queue_push(tp->input_queue, task);
}

gpointer
thread_pool_pop (ThreadPool *tp)
{
    return g_async_queue_pop(tp->output_queue);
}

static void *
thread_pool_func (gpointer data)
{
    ThreadPool *tp;
    gpointer task;

    tp = (ThreadPool *) data;
    g_async_queue_ref(tp->input_queue);
    g_async_queue_ref(tp->output_queue);

    for(;;){
        task = g_async_queue_pop(tp->input_queue);
        if (tp->exit) break;
        tp->worker(tp->output_queue, task);
    }

    g_async_queue_unref(tp->input_queue);
    g_async_queue_unref(tp->output_queue);
}

ThreadPool *
thread_pool_new (guint size, ThreadPoolFun worker)
{
    ThreadPool *tp;
    guint i;

    if (size == 0)
        return NULL;

    tp = g_malloc(sizeof(ThreadPool));
    tp->threads = g_malloc(sizeof(pthread_t) * size);
    tp->input_queue = g_async_queue_new();
    tp->output_queue = g_async_queue_new();
    tp->exit = FALSE;
    tp->worker = worker;
    tp->size = size;

    for (i = 0; i < size; i++) {
        if (pthread_create(&tp->threads[i], NULL, thread_pool_func, tp) != 0){
            g_printerr("Failed to create thread");
        }
    }

    return tp;
}

void
thread_pool_destroy (ThreadPool *tp)
{
    guint i;
    tp->exit = TRUE;
    for (i = 0; i < tp->size; i++){
        // push dummy data
        g_async_queue_push(tp->input_queue, (gpointer) 0x1);
    }
    for (i = 0; i < tp->size; i++){
         pthread_join(tp->threads[i], NULL);
    }
    g_async_queue_unref(tp->input_queue);
    g_async_queue_unref(tp->output_queue);
    g_free(tp->threads);
    g_free(tp);
}

guint
thread_pool_size (ThreadPool *tp)
{
    g_return_val_if_fail(tp, 0);

    return tp->size;
}
