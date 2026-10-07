#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include "../includes/ring_buffer.h"
#include "../includes/http.h"
#include "../includes/thread.h"
#include "../includes/socket.h"

Thread_Pool *pool_init(Thread_Pool *pool, Ring_Buffer *rb, size_t thread_count, size_t queue_capacity)
{
    buffer_init(rb, queue_capacity);
    
    pthread_mutex_init(&pool->lock, NULL);
    pthread_cond_init(&pool->cond, NULL);

    pool->stopping       = false;
    pool->thread_count   = thread_count;
    pool->buffer         = rb;

    for(size_t i=0; i< thread_count; i++)
    {
        pthread_create(&pool->threads[i], NULL, worker, pool);
    }

    return pool;
}

int pool_submit(Thread_Pool *pool, int client_fd)
{
    if (client_fd < 0)
    {
        fprintf(stderr, "%s\n", BAD_CLIENT_FD);
        return -1;
    }
    pthread_mutex_lock(&pool->lock);
    if (!buffer_is_full(pool->buffer))
    {
        buffer_push(pool->buffer, client_fd);
        pthread_cond_signal(&pool->cond);
        pthread_mutex_unlock(&pool->lock);
        return 0;
    }
    pthread_mutex_unlock(&pool->lock);
    fprintf(stderr, "%s\n", BUFFER_FULL);
    close(client_fd);
    return 1;  
}

// sets stopping, joins threads, closes fds, frees resources
void pool_destroy(Thread_Pool *pool)
{
    if(pool)
    {
        pthread_mutex_lock(&pool->lock);
        pool->stopping = true;
        pthread_cond_broadcast(&pool->cond);
        pthread_mutex_unlock(&pool->lock);

        for(int i=0; i < (int)pool->thread_count; i++)
        {
            pthread_join(pool->threads[i], NULL);
        }

        while(!buffer_is_empty(pool->buffer))
        {
            int fd;
            buffer_pop(pool->buffer, &fd);
            close(fd);   
        }
        buffer_free(pool->buffer);
        pthread_mutex_destroy(&pool->lock);
        pthread_cond_destroy(&pool->cond);
    }
}

void *worker(void *arg)
{
    int connection_fd;

    Thread_Pool *pool = (Thread_Pool *)(arg);

    while(1)
    {
        pthread_mutex_lock(&pool->lock);
        while(buffer_is_empty(pool->buffer) && !pool->stopping)
        {
            pthread_cond_wait(&pool->cond, &pool->lock);
        }
        if (pool->stopping)
        {
            pthread_mutex_unlock(&pool->lock);
            break;
        }
        buffer_pop(pool->buffer, &connection_fd);
        pthread_mutex_unlock(&pool->lock);
        
        handle_client(connection_fd);
        close(connection_fd);
    }
    return NULL;
}
