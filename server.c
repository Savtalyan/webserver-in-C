#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080
#define BACKLOG 10
#define SOCKERROR (-1)

int main(int argc, char **argv) 
{
	int socket_fd;
	int client_fd;
	
	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);
		
	char buffer[1024];

	// creating an IPV4 socket, TCP
	socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_fd == -1)
	{
		perror("socket : error while creating socket");
		exit(EXIT_FAILURE);
	}

    int opt = 1;
    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	
	// sockaddr_in contains padding (sin_zero). 
    // If it contains junk data, some systems may reject the bind().
    // So we zero the entire structure first.
	memset(&server_addr, 0, sizeof(server_addr));

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(PORT);
	
	// Most socket functions expect a generic struct sockaddr*.
    // Since we have a sockaddr_in, we cast its address to (struct sockaddr*).
	
	int bind_result = bind(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
	if (bind_result < 0)
	{
		perror("bind: error while binding the server");
		exit(EXIT_FAILURE);
	}
	
	int listen_result = listen(socket_fd, BACKLOG);
	if (listen_result < 0)
	{
		perror("listen: error while entering listening mode");
		exit(EXIT_FAILURE);
	}
	
    printf("Server launched on port %d\n", PORT);

	while (1)
	{
		client_fd = accept(socket_fd, (struct sockaddr*)&client_addr, &client_len);
		if (client_fd == SOCKERROR)
		{
			perror("accept : failed to accept client connection");
			continue;
		}

		int read_result = read(client_fd, buffer, sizeof(buffer) - 1);
		if(read_result <= 0)
        {
            if(read_result == SOCKERROR)
            {
                perror("read: failed to read");
            }
            close(client_fd);
            continue;
        }
        buffer[read_result] = '\0';

        printf("Request: \n %s \n", buffer);

        const char *body = "<h1> Hello from my C HTTP server</h1>";
        char response[2048];

        int len = snprintf(response, sizeof(response),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: %zu\r\n"
            "Connection: close\r\n"
            "\r\n"
            "%s",
            strlen(body), body);
        
        int write_result = write(client_fd, response, len);
		if (write_result == SOCKERROR)
        {
            perror("write: error while writing");
        }
        close(client_fd);
	}

	close(socket_fd);
	return 0;
}
