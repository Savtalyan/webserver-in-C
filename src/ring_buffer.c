#include <stdbool.h>
#include <stdio.h>
#include <sys/types.h>

#include "../includes/ring_buffer.h"

void buffer_init(Ring_Buffer *rb)
{
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
}

bool buffer_push(Ring_Buffer *rb, int data)
{
    if(rb->count == BUFFER_SIZE)
    {
        return false;
    }
    buffer->buffer[buffer->head] = data;
    buffer->head = (buffer->head + 1) % BUFFER_SIZE;
    buffer->count++;
    return true;
}

void buffer_push_overwrite(Ring_Buffer *rb, int data)
{
    buffer->buffer[buffer->head] = data;
    buffer->head = (buffer->head + 1) % BUFFER_SIZE;
    if (buffer->size == BUFFER_SIZE)
    {
        buffer->tail = (buffer->tail + 1) % BUFFER_SIZE;
    }
    else
    {
        buffer->count++;
    }
}

bool buffer_pop(Ring_Buffer *rb, int *data)
{
    if(buffer->count == 0)
    {
        return false;
    }
    *data = buffer->buffer[buffer->tail];
    buffer->tail = (buffer->tail + 1) % BUFFER_SIZE;
    buffer->count--;
    return true;
}

bool buffer_peek(Ring_Buffer *rb, int *data)
{
    if(buffer->count == 0)
    {
        return false;
    }
    *data = buffer->buffer[buffer->tail];
    return true;
}

bool buffer_is_full(Ring_Buffer *rb)
{
    return buffer->count == BUFFER_SIZE;
}

bool buffer_is_empty(Ring_Buffer *rb)
{
    return buffer->count == 0;
}
