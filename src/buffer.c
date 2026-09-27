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
    buffer->temp = realloc(buffer->buff, (buffer->size + needed_size));
    if(buffer->temp == NULL)
    {
        fprintf(stderr, "realloc : %s\n", strerror(errno));
        return -1;
    }
    buffer->buff = buffer->temp;
    buffer->temp = NULL;

    buffer->capacity = buffer->capacity + needed_size;
    buffer->size = buffer->size + needed_size;

    return 0;
}

int buffer_append(Buffer *buffer, const char *data, size_t len)
{
    while(buffer->capacity < len)
    {
        int exp = expand_buffer(buffer, len);
        if(exp != 0)
        {
            return -1;
        }
    }
    memcpy(buffer->buff + (buffer->size - buffer->capacity), data, len);
    buffer->capacity = buffer->capacity - len;
    return 0;
}


void free_buffer(Buffer *buffer)
{
    free(buffer->buff);
    free(buffer);
}
