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

// code exit values
#define SOCKERROR (-1)

// basic status codes 
#define SUCCESS 200
#define NOT_FOUND 404
#define SERVER_ERR 500

// headers 
typedef struct {
    char host[64];
    char content_type[64];
    size_t content_lenght;
} Headers;

// HTTP_Request: http request params
typedef struct {
    char method[10];
    char path[256];
    char version[10];
    Headers headers;
    char body[2048];
} HTTP_Request;


// HTTP_response : http response params
typedef struct {
    char method[10];
    char version[10];
    int statusCode;
    char reason[64];
    char server[64];
    char body[2048];
    Headers headers;
} HTTP_Response;

// Route : basic route structure
typedef struct {
    char *method;
    char *path;
    Acition_Result* (*handler)(const HTTP_Request * request)
} Route;

// Action_Result : function result struct
typedef struct {
    int status_code;
    char *body;
    size_t body_len;
    const char *content_type;
} Acition_Result;


// safe_strncpy : helper function to safely copy n bytes, without code duplication
static void safe_strncpy(char *dest, const char *src, size_t dest_size)
{
    strncpy(dest, src, dest_size - 1);
    dest[dest_size - 1] = '\0';
}

char *server_root(void)
{
    char string[64] = "Welcome to my server. Feel yourself like shit\n";
    return string;
}

char *handle_get_users(void)
{
    // TODO :implement some logic later
    return NULL;
}

// TODO: implement the logic of filling this table based on the routes file
static const Route routing_table[] = {
    { "GET", "/", server_root },
    { "GET", "/users", handle_get_users}
};

#define ROUTES_LENGHT (sizeof(routing_table) / sizeof(routing_table[0]))

// read_request : read the request as byte stream and store it in buff
// return the result or read() function
int readRequest(int client_fd, char *buffer, size_t buff_len)
{
    int read_result = read(client_fd, buffer, buff_len);
    return read_result;
}

// parese_request : parse HTTP request into participants
// [HTTP version, path, method, headers, body]
HTTP_Request parseRequest(const char *buffer, size_t buff_len)
{
    HTTP_Request request;
    
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

    safe_strncpy(request.method, method, sizeof(request.method));
    safe_strncpy(request.path, path, sizeof(request.path));
    safe_strncpy(request.version, version, sizeof(request.version));

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
                safe_strncpy(request.headers.host, value, sizeof(request.headers.host));
            }
            else if (strcmp(key, "Content-Type") == 0)
            {
                safe_strncpy(request.headers.content_type, value, sizeof(request.headers.content_type));
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
// build the route struct
Route *build_route(const HTTP_Request *request, Route *route)
{
    safe_strncpy(route->method, request->method, sizeof(route->method));
    safe_strncpy(route->path, request->path, sizeof(route->path));
    route->handler = NULL;
    return route;
}

// route_resolver : resolve the route, look up in routes_table 
Route *route_resolver(Route *route)
{
    for (size_t i=0; i < ROUTES_LENGHT; i++)
    {
        if (strcmp(routing_table[i].method, route->method) && strcmp(routing_table[i].path, route->path))
        {
            return (Route*)&routing_table[i];
        }
    }
    return NULL;
}

Acition_Result *route_handler(const Route* route, const HTTP_Request *request, Acition_Result *action_result)
{
    Acition_Result *result = route->handler(request);
    // TODO : implemnt checks for the return value
    return result;
}

HTTP_Response *handleRequest(const HTTP_Request *request)
{
    int STATUS_CODE;

    
    HTTP_Response *response = malloc(sizeof(HTTP_Response));
    if(response == NULL)
    {
        perror("malloc: failed to allocate memory for response struct");
        exit(EXIT_FAILURE);
    }
    memset(response, 0, sizeof(*response));

    // we need to pass the request, to the correct route, and to the correct function. 
    //typedef struct {
    // char *method;
    // char *path;
    // HTTP_Response* (*handler)(const HTTP_Request * request)
        // } Route;
    Route *route = malloc(sizeof(Route));
    if (route == NULL)
    {
        perror("failed to allocate memory for route");
        exit(EXIT_FAILURE);
    }
    // filling in the route structure
    safe_strncpy(route->method, request->method, sizeof(route->method));
    safe_strncpy(route->path, request->path, sizeof(route->path));

    // need to call the handle route function here
    // it will accept the route half-constructed structure, assign the corresponding handler function to it, 
    // and call the handlre function with 

    safe_strncpy(response->version, request->version, sizeof(response->version));
    safe_strncpy(response->headers.content_type, request->headers.content_type, sizeof(response->headers.content_type));


}

