#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <sys/types.h>
#include <stdbool.h>


typedef struct {
    int head;
    int tail;
    int count;
    int buffer_capacity;
    int *buffer;
} Ring_Buffer;

void buffer_init(Ring_Buffer *rb, size_t buff_capacity);
bool buffer_push(Ring_Buffer *rb, int data);
void buffer_push_overwrite(Ring_Buffer *rb, int data);
bool buffer_pop(Ring_Buffer *rb, int *data);
bool peek(Ring_Buffer *rb, int *data);
bool buffer_is_empty(Ring_Buffer *rb);
bool buffer_is_full(Ring_Buffer *rb);
void buffer_free(Ring_Buffer *rb);


#endif // RING_BUFFER_H
