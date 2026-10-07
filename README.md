# C HTTP Server

A small multithreaded HTTP/1.1 server written in C using POSIX sockets and pthreads.
Incoming connections are accepted on the main thread, queued in a ring buffer, and handled by a fixed pool of worker threads.

## Features

- IPv4/IPv6 listening socket via `getaddrinfo`
- Thread pool with a bounded ring-buffer job queue
- HTTP request parsing (request line, `Host`, `Content-Type`, `Content-Length`, body)
- Static routing table mapping method + path to handler functions
- Response serialization into a growable byte buffer
- Correct handling of partial writes and `EINTR`

## Project structure

```
.
├── Makefile
├── includes/
│   ├── buffer.h
│   ├── http.h
│   ├── ring_buffer.h
│   ├── socket.h
│   ├── thread.h
│   └── utils.h
└── src/
    ├── main.c          # entry point: sets up the pool, listens, accepts
    ├── socket.c        # listening socket, accept, per-client handling
    ├── thread.c        # thread pool: init, submit, workers, shutdown
    ├── ring_buffer.c   # fixed-capacity FIFO queue of client fds
    ├── http.c          # parsing, routing, handlers, response serialization
    ├── buffer.c        # growable byte buffer used to build responses
    └── utils.c         # string helpers, CRLF search, allocation checks
```

## Requirements

- A POSIX system (Linux, macOS, BSD)
- A C11 compiler (`gcc` or `clang`)
- `make`

## Building

```sh
make            # release build -> ./server
make DEBUG=1    # debug build with AddressSanitizer and UBSan
make clean      # remove build artifacts
```

Object files go to `build/`. Header dependencies are tracked automatically, so editing a header rebuilds only the files that include it.

## Running

```sh
make run
# or
./server
```

The server listens on port `8080` by default.

## Usage

```sh
curl -i http://localhost:8080/
```

```
HTTP/1.1 200 OK
Content-Type: text/plain
Content-Length: 22
Connection: close

Welcome to my server.
```

Raw request with `nc`:

```sh
printf 'GET / HTTP/1.1\r\nHost: localhost\r\n\r\n' | nc localhost 8080
```

### Routes

| Method | Path     | Response                                 |
|--------|----------|------------------------------------------|
| GET    | `/`      | `200 OK` with a welcome message          |
| GET    | `/users` | `500` (handler not implemented yet)      |
| any    | other    | `404 Not Found`                          |

Malformed requests get `400 Bad Request`.

## How it works

1. `main` initializes the thread pool, which starts the worker threads, and opens the listening socket.
2. The main thread loops on `accept` and hands each client fd to `pool_submit`, which pushes it onto the ring buffer and signals a worker.
3. A worker pops an fd, releases the lock, and calls `handle_client`.
4. `handle_client` reads the request, parses it, resolves a route, runs the handler, serializes the response, and sends it.
5. The worker closes the connection and waits for the next job.

If the queue is full, the new connection is closed immediately.

## Configuration

These are compile-time constants in `src/main.c`:

| Constant        | Default | Meaning                         |
|-----------------|---------|---------------------------------|
| `PORT`          | `8080`  | Listening port                  |
| `BACKLOG`       | `10`    | `listen()` backlog              |
| `THREAD_COUNT`  | `4`     | Number of worker threads        |
| `POOL_CAPACITY` | `4`     | Max queued connections          |

## Adding a route

Write a handler with this signature in `http.c`:

```c
Action_Result *my_handler(const HTTP_Request *request);
```

The handler returns a heap-allocated `Action_Result` with a heap-allocated `body`. Both are freed by the caller. Returning `NULL` produces a `500` response.

Then add the handler to `routing_table`:

```c
{ "GET", "/hello", my_handler },
```

## Limitations

- The request is read with a single `read()` call, so requests split across multiple TCP packets may be rejected.
- One request per connection (`Connection: close`), with no keep-alive.
- Request and response bodies are limited by fixed-size buffers.
- Routes are hardcoded and matched exactly, with no query-string or path-parameter support.
- No read timeout, so idle clients can tie up a worker.
- No graceful shutdown on `SIGINT`/`SIGTERM`.

## Roadmap

- [ ] Load the routing table from a routes file
- [ ] Implement `/users`
- [ ] Read requests in a loop until headers and body are complete
- [ ] Add `SO_RCVTIMEO` on client sockets
- [ ] Graceful shutdown via signal handling
- [ ] Keep-alive support
