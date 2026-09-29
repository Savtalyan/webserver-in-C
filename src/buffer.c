#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "../includes/buffer.h"
#include "../includes/utils.h"


Buffer *new_buffer(size_t base_size)
{
    Buffer *buffer = malloc(sizeof(Buffer));
    if(buffer == NULL)
    {
        fprintf(stderr, "malloc %s\n", strerror(errno));
        return NULL;
    }
    
    char *buff = malloc(sizeof(char) * base_size);
    if(buff == NULL)
    {
        fprintf(stderr, "malloc %s\n", strerror(errno));
        free(buffer);
        return NULL;
    }

    buffer->buff = buff;
    buffer->size = base_size;
    buffer->capacity = buffer->size;

    return buffer;
}

int expand_buffer(Buffer *buffer, size_t needed_size)
{
    char *temp = realloc(buffer->buff, (buffer->size + needed_size));
    if(temp == NULL)
    {
        fprintf(stderr, "realloc : %s\n", strerror(errno));
        return -1;
    }
    buffer->buff = temp;;
    buffer->capacity = buffer->capacity + needed_size;
    buffer->size = buffer->size + needed_size;

    return 0;
}

int buffer_append(Buffer *buffer, const char *data, size_t len)
{
    if (len == 0)
    {
        return 0;
    }

    if (buffer->capacity < len)
    {
        size_t grow = len > buffer->size ? len : buffer->size;
        if (expand_buffer(buffer, grow) != 0)
        {
            return -1;
        }
    }

    memcpy(buffer->buff + (buffer->size - buffer->capacity), data, len);
    buffer->capacity = buffer->capacity - len;
    return 0;
}

size_t buffer_used(const Buffer *buffer)
{
    return buffer->size - buffer->capacity;
}

void free_buffer(Buffer *buffer)
{
    if(buffer == NULL)
    {
        return;
    }

    free(buffer->buff);
    free(buffer);
}
