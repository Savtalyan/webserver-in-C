# Simple HTTP Server (C)

A small HTTP/1.1 server written from scratch in C, built as a learning
project
The goal isn't production readiness; it's understanding sockets, HTTP,
memory ownership, and (soon) threading, by implementing all of it by
hand rather than relying on a library.


## Architecture

The project is split into independent modules, each with a header
(contract) and a matching source file (implementation). A module only
exposes what other modules actually need to call; everything else is
kept internal.

```
includes/
  buffer.h    - growable byte buffer
  socket.h    - listening socket + accept
  http.h      - HTTP request/response types and handling
  utils.h     - small shared helpers
src/
  buffer.c
  socket.c
  http.c
  utils.c
```

### `buffer` — growable byte buffer

A small dynamic-array-style buffer (`Buffer`: pointer, total size,
remaining capacity), used to assemble a serialized HTTP response
without needing to know its final size up front.

- `new_buffer(base_size)` — allocate a buffer with an initial capacity.
- `buffer_append(buffer, data, len)` — append bytes, growing the
  buffer automatically if it doesn't currently fit.
- `expand_buffer(buffer, needed_size)` — grows the underlying
  allocation via `realloc`; used internally by `buffer_append`.
- `free_buffer(buffer)` — releases the buffer's data and the struct
  itself. Must be called on every `Buffer*` once it's no longer needed.

Generic — has no knowledge of HTTP or sockets.

### `socket` — listening socket and connection acceptance

Thin wrappers around the raw POSIX socket calls needed to run a TCP
server:

- `create_listening_socket(port, backlog)` — resolves an address via
  `getaddrinfo`, creates a socket, binds it, and puts it into listening
  mode. Returns the listening fd, or `-1` on failure.
- `accept_connection(listen_fd)` — accepts a single pending connection
  and returns the new connection's fd, or `-1` on failure (check
  `errno` for the reason — retry logic is the caller's responsibility,
  not this function's).

Deliberately minimal: no accept loop, no retry policy, no HTTP
knowledge. That logic belongs in `main`, once it exists.

### `http` — HTTP semantics

Everything related to understanding and producing HTTP messages:

- `parse_request(buffer, buff_len, request)` — parses a raw byte
  buffer (as read from a socket) into a structured `HTTP_Request`.
  Does not assume the input is null-terminated.
- `handle_request(request)` — resolves a route from a small static
  routing table and dispatches to the matching handler, producing an
  `HTTP_Response`. Unmatched routes and failing handlers are turned
  into proper 404/500 responses rather than propagating `NULL`.
- `serialize_response(response)` — formats an `HTTP_Response` into
  wire-format bytes (status line, headers, blank line, body) using the
  `buffer` module, ready to be written to a socket.
- Route handlers (`server_root`, `handle_get_users`) and the routing
  internals (`build_route`, `route_resolver`, `route_handler`) that
  support `handle_request`.

### `utils` — shared helpers

- `safe_strncpy` — bounds-checked string copy that always
  null-terminates.
- `malloc_check` — aborts with an error if an allocation failed.
- `find_crlf` — locates a `\r\n` sequence within a buffer using an
  explicit length, without relying on null-termination.

## Design notes

- **Ownership is explicit.** Every function that returns a
  heap-allocated pointer documents, in its header comment, who owns
  the result and who is responsible for freeing it.
- **No `exit()` from request-handling code.** A malformed request from
  one client should never be able to bring down the whole server —
  parsing failures return an error code to the caller instead.
- **Modules don't leak into each other.** `socket` knows nothing about
  HTTP; `http` knows nothing about sockets; `buffer` knows about
  neither. `main` (once written) is the only place that wires them
  together.
- **The routing table is read-only** (`static const`), which keeps it
  safe to read from multiple threads without synchronization once
  threading is introduced — as long as it stays read-only after
  startup.

## Known issues

- `handle_request` has a bug in its route-match check (`=` instead of
  `==`), which currently makes every request skip the "not found"
  path and dereference a `NULL` handler.
- The `static` functions declared in `utils.h` are only defined in
  `utils.c`; any other file that calls them will fail to link as-is.
- No `main.c` / accept loop yet — the pieces exist but aren't wired
  together into a running program.
- Single request per connection is assumed; no keep-alive support.

## Roadmap

1. Fix the issues above and get a single-threaded request/response
   cycle working end-to-end (`create_listening_socket` → accept loop →
   `read` → `parse_request` → `handle_request` → `serialize_response`
   → `write` with partial-write handling → close).
2. Verify against a real client (`curl localhost:8080/`).
3. Add a hand-built thread pool: a fixed set of worker threads, a
   custom job queue (mutex + condition variable) fed by an acceptor
   loop, replacing the single-threaded dispatch.
4. Everything beyond that (event-loop I/O, connection keep-alive,
   dynamic routing) is explicitly out of scope for now — this project
   is about learning C, sockets, and concurrency primitives, not
   building a production server.

## Building

Not yet stable enough for a fixed build command — see Known issues.
Once those are resolved, something like:

```sh
gcc -Iincludes src/*.c -o server
```
