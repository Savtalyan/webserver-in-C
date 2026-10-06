#ifndef THREAD_H
#define THREAD_H

#include <sys/types.h>
#include <stdbool.h>

#include "ring_buffer.h"

#define THREADS_COUNT 4
#define BUFFER_FULL "Buffer is full\n"
#define BAD_CLIENT_FD "Bad client fd\n"

typedef struct {
    pthread_mutex_t     lock;
    pthread_cond_t      cond;
    bool                stopping;
    size_t              thread_count;
    Ring_Buffer         *buffer;
    pthread_t           threads[];

} Thread_Pool;

// creates the pool, initialized the ring buffer, mutex, cond
// Returns : Thread_Pool
Thread_Pool *pool_init(Thread_Pool *pool, Ring_Buffer *rb, size_t thread_count, size_t queue_capacity);

// accepts a connection, locks the mutex, pushes the connection
// fd to the buffer, sends pthread_cond_signal, releases mutex
int pool_submit(Thread_Pool *pool, int client_fd);

// sets stopping, joins threads, closes fds, frees resources
void pool_destroy(Thread_Pool *pool);


// thread inside wrapper
// runs infinite loop, puts the thread to sleep
// until the condition predicate is met, wakes up 
// runs the function
void *worker(void *arg);


#endif // THREAD_H
