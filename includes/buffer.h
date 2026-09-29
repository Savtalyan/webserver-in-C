#ifndef BUFFER_H
#define BUFFER_H

#include <stddef.h>

#define BASE_SIZE 32

typedef struct Buffer {
    char   *buff;      // pointer to the allocated memory
    size_t  size;      // total allocated size (whole buffer)
    size_t  capacity;  // remaining free space (shrinks with each write)
} Buffer;

// allocates buff of base_size, size = capacity = base_size; NULL on failure
Buffer *new_buffer(size_t base_size);

// grows buff by needed_size via realloc into a LOCAL pointer, committed
// only if realloc succeeded. Returns 0 on success, -1 on failure.
int expand_buffer(Buffer *buffer, size_t needed_size);

// appends len bytes; grows the buffer if needed. 0 on success, -1 on failure.
int buffer_append(Buffer *buffer, const char *data, size_t len);

// number of bytes written so far (what you pass to send())
size_t buffer_used(const Buffer *buffer);

// releases buff and the struct itself. Safe on NULL.
void free_buffer(Buffer *buffer);

#endif // BUFFER_H