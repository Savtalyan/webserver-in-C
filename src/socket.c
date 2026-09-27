#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>


int create_listening_socket(const char *port, const int backlog)
{
	int socket_fd;
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
		fprintf(stderr, "Error: %s\n", gai_strerror(res));
		return -1;
	}

	for(rp=results; rp != NULL; rp = rp->ai_next)
	{
		socket_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		
		if (socket_fd == -1)
		{
			continue;
		}

		if (bind(socket_fd, rp->ai_addr, rp->ai_addrlen) == 0)
		{
			break;
		}
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
		fprintf(stderr, "Failed to put the socket into listening mode\n");
		close(socket_fd);
		return -1;
	}
	return socket_fd;	
}

int accept_connection(const int listen_fd)
{
	// TODO: add peer address retrieval
	int conn_fd = accept(listen_fd, NULL, NULL);
	if (conn_fd == -1)
	{
		fprintf(stderr, "failed to accept a connection,%s\n", strerror(errno));
		return -1;
	}
	return conn_fd;
}

