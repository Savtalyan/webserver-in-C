#include <stdbool.h>
#include <stdio.h>
#include <sys/types.h>

#include "../includes/ring_buffer.h"

void buffer_init(Ring_Buffer *rb, size_t buff_capacity)
{
    rb->buffer[buff_capacity];
    rb->buffer_capacity = buff_capacity;
    rb->head            = 0;
    rb->tail            = 0;
    rb->count           = 0;
}

bool buffer_push(Ring_Buffer *rb, int data)
{
    if(rb->count == rb->buffer_capacity)
    {
        return false;
    }
    rb->buffer[rb->head] = data;
    rb->head = (rb->head + 1) % rb->buffer_capacity;
    rb->count++;
    return true;
}

void buffer_push_overwrite(Ring_Buffer *rb, int data)
{
    rb->buffer[rb->head] = data;
    rb->head = (rb->head + 1) % rb->buffer_capacity;
    if (rb->count == rb->buffer_capacity)
    {
        rb->tail = (rb->tail + 1) % rb->buffer_capacity;
    }
    else
    {
        rb->count++;
    }
}

bool buffer_pop(Ring_Buffer *rb, int *data)
{
    if(rb->count == 0)
    {
        return false;
    }
    *data = rb->buffer[rb->tail];
    rb->tail = (rb->tail + 1) % rb->buffer_capacity;
    rb->count--;
    return true;
}

bool buffer_peek(Ring_Buffer *rb, int *data)
{
    if(rb->count == 0)
    {
        return false;
    }
    *data = rb->buffer[rb->tail];
    return true;
}

bool buffer_is_full(Ring_Buffer *rb)
{
    return rb->count == rb->buffer_capacity;
}

bool buffer_is_empty(Ring_Buffer *rb)
{
    return rb->count == 0;
}
