#ifndef THREAD_H
#define THREAD_H

#include <sys/types.h>

#define THREADS_COUNT 4


typedef struct {
    pthread_mutex_t     lock;
    pthread_cond_t      not_empty;
    pthread_cond_t      not_full;
    bool                stopping;

    size_t              threads_count;
    Queue               *queue;  // will import the queue from another module
    pthread_t           threads[];

} Thread_Pool;

// allocates the pool, initializes the mutex, conditions and queue
// Starts the threads 
Thread_Pool *pool_create(size_t n_threads, size_t queue_capacity);

// Locks, pushes the fd, signals not_empty, unlocks
int pool_submit(Thread_Pool *pool, int client_fd);

// sets stopping, joins threads, closes fds, frees resources
void pool_destoroy(Thread_Pool *pool);



static void *worker_main(void *arg);

static int queue_push(void *args);
static int queue_pop(void *args);

#endif // THREAD_H
