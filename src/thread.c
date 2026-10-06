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
    pool->buffer = rb;
    return pool;

}

int pool_submit(Thread_Pool *pool, int client_fd)
{
    if (client_fd < 0)
    {
        perror(BAD_CLIENT_FD);
        return -1;
    }
    pthread_mutex_lock(&pool->lock);
    if (!buffer_is_full(pool->buffer))
    {
        pool_submit(pool, client_fd);
        pthread_cond_signal(&pool->cond);
        pthread_mutex_unlock(&pool->lock);
        return 0;
    }
    perror(BUFFER_FULL);
    return 1; 
}

// sets stopping, joins threads, closes fds, frees resources
void pool_destroy(Thread_Pool *pool)
{
    if(pool)
    {
        pool->stopping = true;
        for(int i=0; i < (int)pool->thread_count; i++)
        {
            pthread_join(pool->threads[i], NULL);
        }
        int fd;
        while(!buffer_is_empty(pool->buffer))
        {
            int fd = buffer_pop(pool->buffer, &fd);
            close(fd);   
        }
    }
}

void *worker(void *arg)
{
    int connection_fd;

    Thread_Pool *pool = (Thread_Pool *)(arg);

    while(1)
    {
        pthread_mutex_lock(&pool->lock);
        while(buffer_is_empty(pool->buffer))
        {
            pthread_cond_wait(&pool->cond, &pool->lock);
        }
        buffer_pop(pool->buffer, &connection_fd);
        handle_client(connection_fd);
    }
    return NULL;
}


void handle_client(int client_fd)
{
    char read_buffer[READ_BUFFER_SIZE];

    ssize_t n = read_request(client_fd, read_buffer, sizeof(read_buffer));
    if (n <= 0)
    {
        if (n < 0)
        {
            perror("read: failed to read");
        }
        return;
    }

    HTTP_Request request;
    if (parse_request(read_buffer, (size_t)n, &request) != 0)
    {
        static const char bad[] =
            "HTTP/1.1 400 Bad Request\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 12\r\n"
            "Connection: close\r\n"
            "\r\n"
            "Bad Request\n";
        send_all(client_fd, bad, sizeof(bad) - 1);
        return;
    }
