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

// basic status codes 
#define SUCCESS 200
#define NOT_FOUND 404
#define SERVER_ERR 500

typedef struct {
    char host[64];
    char content_type[64];
    size_t content_lenght;
} Headers;

typedef struct {
    char method[10];
    char path[256];
    char version[10];
    Headers headers;
    char body[2048];
} requestStruct;

typedef struct {
    int statusCode;
    char body[2048];
    Headers headers;
} httpResponse;

// receiving request
// int readRequest(int client_fd, char *buffer, size_t buff_len);
// requestStruct parseRequest(char *buffer, size_t buff_len);
// httpResponse handleRequest(requestStruct request);


int readRequest(int client_fd, char *buffer, size_t buff_len)
{
    int read_result = read(client_fd, buffer, buff_len);
    return read_result;
}

requestStruct parseRequest(char *buffer, size_t buff_len)
{
    requestStruct request;
    
    char *line_end = strstr(buffer, "\r\n");

    if (line_end == NULL)
    {
        perror("Corrupted or incomplete request");
        // TODO: handle this case, and add specific error type
        exit(EXIT_FAILURE);
    }
    size_t line_len = line_end - buffer;
    char requestLine[64];

    if (line_len > sizeof(requestLine))
    {
        perror("Request line too long");
        // TODO: handle this case and add specific error type
        exit(EXIT_FAILURE);
    }
    
    memcpy(requestLine, buffer, line_len);
    requestLine[line_len] = '\0';

    char *method = strtok(requestLine, " ");
    char *path = strtok(NULL, " ");
    char *version = strtok(NULL, " ");

    if (method == NULL || path == NULL || version == NULL)
    {
        perror("Malformed request string");
        // TODO: handle this case, and add specific error type
        exit(EXIT_FAILURE);
    }

    strncpy(request.method, method, sizeof(request.method) - 1);
    request.method[sizeof(request.method) -1] = '\0';

    strncpy(request.path, path, sizeof(request.path - 1));
    request.path[sizeof(request.path) - 1] = '\0';

    strncpy(request.version, version, sizeof(request.version - 1));
    request.version[sizeof(request.version) - 1] = '\0';

    char *pos = line_end + 2;
    
    while(1)
    {
        char *next_line_end = strstr(pos, "\r\n");
        if (next_line_end == NULL)
        {
            perror("Incomplete headers");
            // TODO: handle this case and add specific error type
            exit(EXIT_FAILURE);
        }

        size_t header_line_len = next_line_end - pos;

        if (header_line_len == 0)
        {
            pos = next_line_end + 2;
            break;
        }

        char header_line[256];
        if (header_line_len >= sizeof(header_line))
        {
            perror("header line too long");
            // TODO: handle this case and add specific error type
            exit(EXIT_FAILURE);
        }
        memcpy(header_line, pos, header_line_len);
        header_line[header_line_len] = '\0';

        char *colon = strstr(header_line, ": ");

        if (colon != NULL)
        {
            *colon = '\0';
            char *key = header_line;
            char *value = colon + 2;
        

            if (strcmp(key, "Host") == 0)
            {
                strncpy(request.headers.host, value, sizeof(request.headers.host) - 1);
                request.headers.host[sizeof(request.headers.host) - 1] = '\0';
            }
            else if (strcmp(key, "Content-Type") == 0)
            {
                strncpy(request.headers.content_type, value, sizeof(request.headers.content_type) - 1);
                request.headers.content_type[sizeof(request.headers.content_type) - 1] = '\0';
            }
            else if (strcmp(key, "Content-Length") == 0)
            {
                request.headers.content_lenght = (size_t)atol(value);
            }
        }
        pos = next_line_end + 2;
    }
    size_t body_len = request.headers.content_lenght;

    if (body_len > 0)
    {
        if (body_len >= sizeof(request.body))
        {
            perror("request body too long");
            exit(EXIT_FAILURE);
        }

        size_t bytes_available = buff_len - (pos - buffer);
        if (bytes_available < body_len)
        {
            perror("incomplete body - not all bytes received");
            exit(EXIT_FAILURE);
        }

        memcpy(request.body, pos, body_len);
        request.body[body_len] = '\0';
    }
    else
    {
        request.body[0] = '\0';
    }

    return request;
}



int main(int argc, char **argv) 
{
	int socket_fd;
	int client_fd;
	
	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);
		
	char read_buffer[2048];
    char write_buffer[2048];

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

        
		int read_result = read(client_fd, read_buffer, sizeof(read_buffer) - 1);
		if(read_result <= 0)
        {
            if(read_result == SOCKERROR)
            {
                perror("read: failed to read");
            }
            close(client_fd);
            continue;
        }
        read_buffer[read_result] = '\0';

        printf("Request: \n %s \n", read_buffer);

        close(client_fd);
	}

	close(socket_fd);
	return 0;
}
