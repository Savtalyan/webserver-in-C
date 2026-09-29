#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../includes/utils.h"

void safe_strncpy(char *dest, const char *src, size_t dest_size)
{
    if (dest_size == 0)
    {
        return;
    }
    size_t n = strnlen(src, dest_size - 1);
    memcpy(dest, src, n);
    dest[n] = '\0';
}

void malloc_check(void *ptr)
{
    if (ptr == NULL)
    {
        perror("malloc: failed to allocate memory");
        exit(EXIT_FAILURE);
    }
}

const char *find_crlf(const char *buffer, size_t buff_len, size_t start_offset)
{
    if (start_offset >= buff_len)
    {
        return NULL;
    }
    for (size_t i = start_offset; i + 1 < buff_len; i++)
    {
        if (buffer[i] == '\r' && buffer[i + 1] == '\n')
        {
            return &buffer[i];
        }
    }
    return NULL;
}