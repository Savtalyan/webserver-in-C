#ifndef REQUEST_H
#define REQUEST_H

#include <stddef.h>

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

/*
 * server_root - handler for GET /.
 *
 * request: read-only, not modified. Currently unused by this handler.
 *
 * Returns a heap-allocated Action_Result. Caller takes ownership and
 * must free() both the returned struct and its body field.
 */
Action_Result *server_root(const HTTP_Request *request);

/*
 * handle_get_users - handler for GET /users.
 *
 * request: read-only, not modified.
 *
 * Currently a placeholder: returns NULL (not yet implemented).
 * route_handler() is responsible for turning a NULL result into a
 * proper SERVER_ERR response before it reaches the client.
 */
Action_Result *handle_get_users(const HTTP_Request *request);

/*
 * parse_request - parses a raw HTTP request from a byte buffer into a
 * structured HTTP_Request.
 *
 * buffer:   raw bytes read from the client socket. Does NOT need to be
 *           null-terminated -- buff_len is authoritative.
 * buff_len: number of valid bytes in buffer.
 * request:  caller-owned HTTP_Request to populate. Must not be NULL.
 *           On failure, contents are unreliable -- do not use them.
 *
 * Returns 0 on success, -1 if the request is malformed or incomplete.
 * Does not take ownership of buffer or request; caller retains both.
 *
 * Thread-safety: safe to call concurrently as long as each call gets
 * its own buffer and its own HTTP_Request.
 */
int parse_request(const char *buffer, size_t buff_len, HTTP_Request *request);

/*
 * build_route - fills in a Route's method/path from a parsed request,
 * as a lookup key for route_resolver. Does not set handler (left NULL).
 *
 * request: read-only, not modified.
 * route:   caller-owned Route to fill in. Must not be NULL.
 */
void build_route(const HTTP_Request *request, Route *route);

/*
 * route_resolver - looks up the routing table for an entry matching
 * route's method and path.
 *
 * route: read-only key to match against (method + path); its handler
 *        field is ignored.
 *
 * Returns a pointer into the static routing table (not caller-owned,
 * do not free) on match, or NULL if no route matches.
 */
Route *route_resolver(Route *route);

/*
 * route_handler - invokes the matched route's handler and guarantees
 * a non-NULL result.
 *
 * route:   a resolved route (as returned by route_resolver), not NULL.
 * request: read-only, passed through to the handler.
 *
 * Returns a heap-allocated Action_Result. Caller takes ownership and
 * must free() both the struct and its body. If the handler itself
 * returns NULL, this constructs a SERVER_ERR result instead of
 * propagating NULL.
 */
Action_Result *route_handler(const Route *route, const HTTP_Request *request);

/*
 * handle_request - resolves a route for the given request and produces
 * a response, dispatching to the matching handler internally.
 *
 * request: caller-owned, read-only, not modified.
 *
 * Returns a heap-allocated HTTP_Response*. Caller takes ownership and
 * must free() it. Never returns NULL -- an unmatched route or a failing
 * handler is represented as a 404/500 response, not a NULL pointer.
 *
 * Thread-safety: safe to call concurrently. The only state shared
 * across calls is the routing table, which is read-only after startup
 * (static const) -- revisit this guarantee if routes ever become
 * mutable at runtime.
 */
HTTP_Response *handle_request(const HTTP_Request *request);

#endif // REQUEST_H