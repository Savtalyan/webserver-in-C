#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "../includes/http.h"
#include "../includes/utils.h"
#include "../includes/buffer.h"

// TODO: implement the logic of filling this table based on the routes file
static const Route routing_table[] = {
    { "GET", "/", server_root },
    { "GET", "/users", handle_get_users}
};

#define ROUTES_LENGHT (sizeof(routing_table) / sizeof(routing_table[0]))
#define CRLF "\r\n"
#define MAXBODYLEN 4096


Action_Result *server_root(const HTTP_Request *request)
{
    (void)request;

    Action_Result *result = malloc(sizeof(Action_Result));
    malloc_check(result);

    const char *message = "Welcome to my server. Feel yourself like shit\n";
    result->body = strdup(message);
    malloc_check(message);
    result->body_len = strlen(message);
    result->status_code = SUCCESS;
    result->content_type = "text/plain";

    return result;
}

Action_Result *handle_get_users(const HTTP_Request *request)
{
    (void)request;
    // TODO :implement some logic later
    return NULL;
}

// read_request : read the request as byte stream and store it in buff
// return the result or read() function
int read_request(int client_fd, char *buffer, size_t buff_len)
{
    int read_result = read(client_fd, buffer, buff_len);
    return read_result;
}

// parese_request : parse HTTP request into participants
// [HTTP version, path, method, headers, body]
int parse_request(const char *buffer, size_t buff_len, HTTP_Request *request)
{
    char *line_end = find_crlf(buffer, buff_len, 0);

    if (line_end == NULL)
    {
        fprintf(stderr, "Corrupted or incomplete request\n");
        // TODO: handle this case, and add specific error type
        return -1;
    }

    size_t line_len = line_end - buffer;
    char requestLine[64];

    if (line_len > sizeof(requestLine))
    {
        fprintf(stderr, "Request line too long\n");
        return -1;
    }
    
    memcpy(requestLine, buffer, line_len);
    requestLine[line_len] = '\0';

    char *method = strtok(requestLine, " ");
    char *path = strtok(NULL, " ");     
    char *version = strtok(NULL, " ");

    if (method == NULL || path == NULL || version == NULL)
    {
       fprintf(stderr, "Malformed request string\n");
        return -1;
    }

    safe_strncpy(request->method, method, sizeof(request->method));
    safe_strncpy(request->path, path, sizeof(request->path));
    safe_strncpy(request->version, version, sizeof(request->version));

    char *pos = line_end + 2;
    
    while(1)
    {
        char *next_line_end = find_crlf(buffer, buff_len, pos - buffer);
        if (next_line_end == NULL)
        {
            fprintf(stderr, "Inclomplete headers\n");
            return -1;
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
            fprintf(stderr, "header line too long\n");
            return -1;
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
                safe_strncpy(request->headers.host, value, sizeof(request->headers.host));
            }
            else if (strcmp(key, "Content-Type") == 0)
            {
                safe_strncpy(request->headers.content_type, value, sizeof(request->headers.content_type));
            }
            else if (strcmp(key, "Content-Length") == 0)
            {
                request->headers.content_lenght = (size_t)atol(value);
            }
        }
        pos = next_line_end + 2;
    }
    size_t body_len = request->headers.content_lenght;

    if (body_len > 0)
    {
        if (body_len >= sizeof(request->body))
        {
            fprintf(stderr, "request body too long\n");
            return -1;
        }

        size_t bytes_available = buff_len - (pos - buffer);
        if (bytes_available < body_len)
        {
            fprintf(stderr, "incomplete body - not all bytes received\n");
            return -1;
        }

        memcpy(request->body, pos, body_len);
        request->body[body_len] = '\0';
    }
    else
    {
        request->body[0] = '\0';
    }

    return 0;
}
// build the route struct
void build_route(const HTTP_Request *request, Route *route)
{
    safe_strncpy(route->method, request->method, sizeof(route->method));
    safe_strncpy(route->path, request->path, sizeof(route->path));
    route->handler = NULL;
}

// route_resolver : resolve the route, look up in routes_table 
Route *route_resolver(Route *route)
{
    for (size_t i=0; i < ROUTES_LENGHT; i++)
    {
        if (strcmp(routing_table[i].method, route->method) == 0 && 
            strcmp(routing_table[i].path, route->path) == 0)
        {
            return (Route*)&routing_table[i];
        }
    }
    return NULL;
}

Action_Result *route_handler(const Route* route, const HTTP_Request *request)
{
    Action_Result *result = route->handler(request);
    
    if (result == NULL)
    {
        result = malloc(sizeof(Action_Result));
        malloc_check(result);
        result->status_code = SERVER_ERR;
        result->body = strdup("Internal server error\n");
        malloc_check(result->body);
        result->body_len = strlen(result->body);
        result->content_type = "text/plain";
    }
    return result;
}

HTTP_Response *handle_request(const HTTP_Request *request)
{
    Route query;
    build_route(request, &query);

    Route *matched = route_resolver(&query);

    HTTP_Response *response = malloc(sizeof(HTTP_Response));
    malloc_check(response);

    // filling in the route structure
    safe_strncpy(response->version, request->version, sizeof(response->version));
    safe_strncpy(response->headers.content_type, request->headers.content_type,
                sizeof(response->headers.content_type));

    if (matched == NULL)
    {
        response->statusCode = NOT_FOUND;
        safe_strncpy(response->reason, "Not Found", sizeof(response->reason));
        safe_strncpy(response->body, "404 Not Found\n", sizeof(response->body));
    }
    else 
    {
        Action_Result *result = route_handler(matched, request);

        response->statusCode = result->status_code;
        safe_strncpy(response->reason,
                    result->status_code == SUCCESS ? "OK" : "Error",
                    sizeof(response->reason));
        safe_strncpy(response->body, result->body, sizeof(response->body));

        free(result->body);
        free(result);
    }
    return response;
}


Buffer *serialize_response(HTTP_Response *response)
{
    char temp_buff[MAXBODYLEN]; // 4096
    int bytes_written;

    Buffer *buffer = new_buffer(BASE_SIZE);
    if (buffer == NULL)
    {
        return NULL;
    }

    // first line of the response
    bytes_written = snprintf(temp_buff, sizeof(temp_buff), "%s %d %s%s", response->version, 
                                response->statusCode, response->reason, CRLF);

    if(buffer_append(buffer, temp_buff, bytes_written) != 0)
    {
        fprintf(stderr, "failed to append request line");
        free_buffer(buffer);
        return NULL;
    }

    // headers - content type
    bytes_written = snprintf(temp_buff, sizeof(temp_buff), "Content-Type : %s%s", response->headers.content_type, CRLF);

    if(buffer_append(buffer, temp_buff, bytes_written) != 0)
    {
        fprintf(stderr, "failed to append content type");
        free_buffer(buffer);
        return NULL;
    }
    
    // headers - content lenght
    bytes_written = snprintf(temp_buff, sizeof(temp_buff), "Content-Length : %zu%s", response->headers.content_lenght, CRLF);

    if(buffer_append(buffer, temp_buff, bytes_written) != 0)
    {
        fprintf(stderr, "failed to append content lenght");
        free_buffer(buffer);
        return NULL;
    }

    // TODO: add other metadata

    // empty line
    if(buffer_append(buffer, CRLF, strlen(CRLF)) != 0)
    {
        fprintf(stderr, "failed to append the empty line");
        free_buffer(buffer);
        return NULL;
    }

    // body
    size_t body_len = strlen(response->body);

    if(buffer_append(buffer, response->body, body_len) != 0)
    {
        fprintf(stderr, "failed to append response body");
        free_buffer(buffer);
        return NULL;
    }

    return buffer;
}