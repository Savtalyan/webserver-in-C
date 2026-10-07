#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>

#include "../includes/http.h"
#include "../includes/utils.h"
#include "../includes/buffer.h"

// TODO: implement the logic of filling this table based on the routes file
static const Route routing_table[] = {
    { "GET", "/", server_root },
    { "GET", "/users", handle_get_users }
};

#define ROUTES_LENGHT (sizeof(routing_table) / sizeof(routing_table[0]))
#define CRLF "\r\n"
#define MAXBODYLEN 4096

static const char *reason_phrase(int code)
{
    switch (code)
    {
        case SUCCESS:     return "OK";
        case BAD_REQUEST: return "Bad Request";
        case NOT_FOUND:   return "Not Found";
        case SERVER_ERR:  return "Internal Server Error";
        default:          return "Unknown";
    }
}

Action_Result *server_root(const HTTP_Request *request)
{
    (void)request;

    Action_Result *result = malloc(sizeof(Action_Result));
    malloc_check(result);

    const char *message = "Welcome to my server.\n";
    result->body = strdup(message);
    malloc_check(result->body);
    result->body_len = strlen(message);
    result->status_code = SUCCESS;
    result->content_type = "text/plain";

    return result;
}

Action_Result *handle_get_users(const HTTP_Request *request)
{
    (void)request;
    // TODO: implement some logic later
    return NULL;
}

ssize_t read_request(int client_fd, char *buffer, size_t buff_len)
{
    return read(client_fd, buffer, buff_len);
}

static int copy_field(char *dest, size_t dest_size, const char *src)
{
    if (strlen(src) >= dest_size)
    {
        return -1;
    }
    memcpy(dest, src, strlen(src) + 1);
    return 0;
}

// parse_request : [method, path, version, headers, body]
int parse_request(const char *buffer, size_t buff_len, HTTP_Request *request)
{
    memset(request, 0, sizeof(*request));

    const char *line_end = find_crlf(buffer, buff_len, 0);
    if (line_end == NULL)
    {
        fprintf(stderr, "Corrupted or incomplete request\n");
        return -1;
    }

    size_t line_len = (size_t)(line_end - buffer);
    char requestLine[10 + 1 + 256 + 1 + 10 + 1];

    if (line_len >= sizeof(requestLine))
    {
        fprintf(stderr, "Request line too long\n");
        return -1;
    }

    memcpy(requestLine, buffer, line_len);
    requestLine[line_len] = '\0';

    char *save = NULL;
    char *method = strtok_r(requestLine, " ", &save);
    char *path = strtok_r(NULL, " ", &save);
    char *version = strtok_r(NULL, " ", &save);

    if (method == NULL || path == NULL || version == NULL)
    {
        fprintf(stderr, "Malformed request string\n");
        return -1;
    }

    if (copy_field(request->method, sizeof(request->method), method) != 0 ||
        copy_field(request->path, sizeof(request->path), path) != 0 ||
        copy_field(request->version, sizeof(request->version), version) != 0)
    {
        fprintf(stderr, "Request line field too long\n");
        return -1;
    }

    const char *pos = line_end + 2;

    while (1)
    {
        const char *next_line_end = find_crlf(buffer, buff_len, (size_t)(pos - buffer));
        if (next_line_end == NULL)
        {
            fprintf(stderr, "Incomplete headers\n");
            return -1;
        }

        size_t header_line_len = (size_t)(next_line_end - pos);

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

        char *colon = strchr(header_line, ':');
        if (colon != NULL)
        {
            *colon = '\0';
            char *key = header_line;
            char *value = colon + 1;
            while (*value == ' ' || *value == '\t')
            {
                value++;
            }

            if (strcasecmp(key, "Host") == 0)
            {
                safe_strncpy(request->headers.host, value, sizeof(request->headers.host));
            }
            else if (strcasecmp(key, "Content-Type") == 0)
            {
                safe_strncpy(request->headers.content_type, value, sizeof(request->headers.content_type));
            }
            else if (strcasecmp(key, "Content-Length") == 0)
            {
                char *end = NULL;
                unsigned long n = strtoul(value, &end, 10);
                if (end == value || *end != '\0' || value[0] == '-')
                {
                    fprintf(stderr, "invalid Content-Length\n");
                    return -1;
                }
                request->headers.content_lenght = (size_t)n;
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

        size_t bytes_available = buff_len - (size_t)(pos - buffer);
        if (bytes_available < body_len)
        {
            fprintf(stderr, "incomplete body - not all bytes received\n");
            return -1;
        }

        memcpy(request->body, pos, body_len);
        request->body[body_len] = '\0';
    }

    return 0;
}

void build_route(const HTTP_Request *request, Route *route)
{
    safe_strncpy(route->method, request->method, sizeof(route->method));
    safe_strncpy(route->path, request->path, sizeof(route->path));
    route->handler = NULL;
}

const Route *route_resolver(const Route *route)
{
    for (size_t i = 0; i < ROUTES_LENGHT; i++)
    {
        if (strcmp(routing_table[i].method, route->method) == 0 &&
            strcmp(routing_table[i].path, route->path) == 0)
        {
            return &routing_table[i];
        }
    }
    return NULL;
}

Action_Result *route_handler(const Route *route, const HTTP_Request *request)
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

    const Route *matched = route_resolver(&query);

    HTTP_Response *response = calloc(1, sizeof(HTTP_Response));
    malloc_check(response);

    safe_strncpy(response->version, "HTTP/1.1", sizeof(response->version));

    if (matched == NULL)
    {
        response->statusCode = NOT_FOUND;
        safe_strncpy(response->headers.content_type, "text/plain",
                     sizeof(response->headers.content_type));
        safe_strncpy(response->body, "404 Not Found\n", sizeof(response->body));
    }
    else
    {
        Action_Result *result = route_handler(matched, request);

        response->statusCode = result->status_code;
        safe_strncpy(response->headers.content_type,
                     result->content_type ? result->content_type : "text/plain",
                     sizeof(response->headers.content_type));
        safe_strncpy(response->body, result->body ? result->body : "",
                     sizeof(response->body));

        free(result->body);
        free(result);
    }

    safe_strncpy(response->reason, reason_phrase(response->statusCode),
                 sizeof(response->reason));

    response->headers.content_lenght = strlen(response->body);
    return response;
}

static int append_fmt(Buffer *buffer, const char *fmt, ...)
{
    char temp_buff[MAXBODYLEN];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(temp_buff, sizeof(temp_buff), fmt, ap);
    va_end(ap);

    if (n < 0 || (size_t)n >= sizeof(temp_buff))
    {
        fprintf(stderr, "header formatting failed or truncated\n");
        return -1;
    }
    return buffer_append(buffer, temp_buff, (size_t)n);
}

Buffer *serialize_response(HTTP_Response *response)
{
    Buffer *buffer = new_buffer(BASE_SIZE);
    if (buffer == NULL)
    {
        return NULL;
    }

    if (append_fmt(buffer, "%s %d %s" CRLF, response->version,
                   response->statusCode, response->reason) != 0 ||
        append_fmt(buffer, "Content-Type: %s" CRLF, response->headers.content_type) != 0 ||
        append_fmt(buffer, "Content-Length: %zu" CRLF, response->headers.content_lenght) != 0 ||
        append_fmt(buffer, "Connection: close" CRLF) != 0 ||
        buffer_append(buffer, CRLF, strlen(CRLF)) != 0 ||
        buffer_append(buffer, response->body, strlen(response->body)) != 0)
    {
        fprintf(stderr, "failed to serialize response\n");
        free_buffer(buffer);
        return NULL;
    }

    return buffer;
}

ssize_t send_response(const int client_fd, const char *buffer, size_t buff_len)
{
    size_t sent_so_far = 0;

    while(sent_so_far < buff_len)
    {
        ssize_t sent = write(client_fd, buffer + sent_so_far, buff_len - sent_so_far);
        if(sent == -1)
        {
            if (errno == EINTR)
            {
                continue;
            }
            return -1;
        }
        sent_so_far += (size_t)sent;
    }
    return (ssize_t)sent_so_far;
}
