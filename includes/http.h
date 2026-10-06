#ifndef HTTP_H
#define HTTP_H

#include <stddef.h>
#include <sys/types.h>

#include "../includes/buffer.h"

// basic status codes
#define SUCCESS 200
#define BAD_REQUEST 400
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
    char body[4096];
    Headers headers;
} HTTP_Response;

// Action_Result : function result struct
typedef struct {
    int status_code;
    char *body;
    size_t body_len;
    const char *content_type;
} Action_Result;

// Route : basic route structure
typedef struct {
    char method[10];
    char path[256];
    Action_Result *(*handler)(const HTTP_Request *request);
} Route;

/* server_root - handler for GET /.
 * Returns heap-allocated Action_Result; caller frees struct and body. */
Action_Result *server_root(const HTTP_Request *request);

/* handle_get_users - handler for GET /users. Placeholder: returns NULL;
 * route_handler() turns that into a SERVER_ERR response. */
Action_Result *handle_get_users(const HTTP_Request *request);

/* read_request - one read() from client_fd into buffer.
 * Returns read()'s result (bytes read, 0 on EOF, -1 on error). */
ssize_t read_request(int client_fd, char *buffer, size_t buff_len);

/* parse_request - parses raw bytes into *request. buffer need not be
 * NUL-terminated. *request is fully zeroed first, so it is always in a
 * defined state. Returns 0 on success, -1 if malformed/incomplete. */
int parse_request(const char *buffer, size_t buff_len, HTTP_Request *request);

/* build_route - fills route's method/path from request; handler = NULL. */
void build_route(const HTTP_Request *request, Route *route);

/* route_resolver - returns a pointer into the static routing table
 * (do not free / modify) or NULL if no route matches. */
const Route *route_resolver(const Route *route);

/* route_handler - invokes the handler, guaranteeing a non-NULL result.
 * Caller frees the struct and its body. */
Action_Result *route_handler(const Route *route, const HTTP_Request *request);

/* handle_request - produces a heap-allocated HTTP_Response (caller frees).
 * Never returns NULL. Content-Length is filled in from the final body. */
HTTP_Response *handle_request(const HTTP_Request *request);

/* serialize_response - formats response into wire bytes. Caller must call
 * free_buffer() on the result. Use buffer_used() for the length to send.
 * Returns NULL on allocation failure. */
Buffer *serialize_response(HTTP_Response *response);

/* send_response - checks the lenght of the response, and runs a loop
 * to make sure all the byter were sent. 
 * Returns -1 on failure, otherwise the number of bytes written */
ssize_t send_response(const int client_fd, const char *buffer, size_t buff_len);

#endif // HTTP_H
