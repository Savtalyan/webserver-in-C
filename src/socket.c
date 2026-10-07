#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "../includes/socket.h"
#include "../includes/http.h"
#include "../includes/buffer.h"

int create_listening_socket(const char *port, const int backlog)
{
	int socket_fd = -1;
	int res;

	struct addrinfo hints;
	struct addrinfo *results;
	struct addrinfo *rp;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_protocol = 0;

	res = getaddrinfo(NULL, port, &hints, &results);
	if (res != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(res));
		return -1;
	}

	for (rp = results; rp != NULL; rp = rp->ai_next)
	{
		socket_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (socket_fd == -1)
		{
			continue;
		}

		int opt = 1;
		if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
		{
			fprintf(stderr, "setsockopt: %s\n", strerror(errno));
			close(socket_fd);
			continue;
		}

		if (bind(socket_fd, rp->ai_addr, rp->ai_addrlen) == 0)
		{
			break;
		}
		fprintf(stderr, "bind: %s\n", strerror(errno));
		close(socket_fd);
	}
	if (rp == NULL)
	{
		fprintf(stderr, "Failed to bind\n");
		freeaddrinfo(results);
		return -1;
	}

	freeaddrinfo(results);

	if (listen(socket_fd, backlog) == -1)
	{
		fprintf(stderr, "listen: %s\n", strerror(errno));
		close(socket_fd);
		return -1;
	}
	return socket_fd;
}

int accept_connection(const int listen_fd)
{
	// TODO: add peer address retrieval
	int conn_fd;
	do
	{
		conn_fd = accept(listen_fd, NULL, NULL);
	} while (conn_fd == -1 && errno == EINTR);

	if (conn_fd == -1)
	{
		fprintf(stderr, "failed to accept a connection: %s\n", strerror(errno));
		return -1;
	}
	return conn_fd;
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
        send_response(client_fd, bad, sizeof(bad) - 1);
        return;
    }

	HTTP_Response *response = handle_request(&request);

	Buffer *out = serialize_response(response);
	if (out != NULL)
	{
		send_response(client_fd, out->buff, buffer_used(out));
		free_buffer(out);
	}
	free(response);
}