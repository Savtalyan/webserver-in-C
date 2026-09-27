#ifndef BUFFER_H
#define BUFFER_H

#define BASE_SIZE 32

#include <stddef.h>


typedef struct {
    char *buff;       // pointer to the allocated memory
    char *temp;
    size_t  size;         // total allocated size (whole buffer)
    size_t  capacity;      // remaining free space (shrinks with each write)
} Buffer;

struct buffer *new_buffer(size_t base_size);
    // allocates buff of base_size, size = capacity = base_size
    // returns NULL on allocation failure

int expand_buffer(struct buffer *buffer, size_t needed_size);
    // grows buff via realloc (into a LOCAL temp pointer, checked for
    // NULL before committing), updates size and capacity in place
    // returns a status code: success / failure

int buffer_append(struct buffer *buffer, const char *data, size_t len);
    // checks buffer->capacity against len; calls expand_buffer if
    // len > capacity; then copies data into the free region at
    // (buffer->buff + used-so-far) and shrinks capacity by len
    // returns a status code: success / failure (propagates
    // expand_buffer's failure if growth was needed and failed)

void free_buffer(struct buffer *buffer);
    // releases buff and the struct itself -- you'll need this once
    // serialize_response's caller is done sending the response,
    // or you've got a new leak the moment this module exists

#endif // BUFFER_H