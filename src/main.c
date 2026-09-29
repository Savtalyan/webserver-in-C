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

#define PORT "8080"
#define BACKLOG 10
#define READ_BUFFER_SIZE 4096

static int send_all(int fd, const char *data, size_t len)
{
    size_t sent = 0;
    while (sent < len)
    {
        ssize_t n = write(fd, data + sent, len - sent);
        if (n < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            perror("write");
            return -1;
        }
        sent += (size_t)n;
    }
    return 0;
}

static void handle_client(int client_fd)
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

    HTTP_Response *response = handle_request(&request);
    Buffer *out = serialize_response(response);
    free(response);

    if (out == NULL)
    {
        return;
    }
    send_all(client_fd, out->buff, buffer_used(out));
    free_buffer(out);
}

int main(void)
{
    signal(SIGPIPE, SIG_IGN);

    int listen_fd = create_listening_socket(PORT, BACKLOG);
    if (listen_fd < 0)
    {
        exit(EXIT_FAILURE);
    }

    printf("Server launched on port %s\n", PORT);

    while (1)
    {
        int client_fd = accept_connection(listen_fd);
        if (client_fd < 0)
        {
            continue;
        }
        handle_client(client_fd);
        close(client_fd);
    }

    close(listen_fd);
    return 0;
}