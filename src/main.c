#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "../includes/socket.h"
#include "../includes/http.h"
#include "../includes/buffer.h"
#include "../includes/thread.h"

#define PORT "8080"
#define BACKLOG 10
#define READ_BUFFER_SIZE 4096

#define THREAD_COUNT 4
#define POOL_CAPACITY 4


int main(void)
{
    Thread_Pool *t_pool;
    Ring_Buffer *rb;

    pool_init(t_pool, rb, THREAD_COUNT, POOL_CAPACITY);

    signal(SIGPIPE, SIG_IGN);

    int listen_fd = create_listening_socket(PORT, BACKLOG);
    if (listen_fd < 0)
    {
        exit(EXIT_FAILURE);
    }

    printf("Server launched on port %s\n", PORT);

    while(1)
    {
        int connection_fd = accept_connection(listen_fd);
        if(connection_fd > -1)
        {
            pool_submit(t_pool, connection_fd);
        }
        else
        {
            continue;
        }
    }   
    pool_destroy(t_pool); 
    return 0;
}
