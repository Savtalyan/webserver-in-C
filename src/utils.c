#include <stdlib.h>

static void safe_strncpy(char *dest, const char *src, size_t dest_size)
{
    strncpy(dest, src, dest_size - 1);
    dest[dest_size - 1] = '\0';
}

static void malloc_check(void *ptr)
{
    if (ptr == NULL)
    {
        perror("malloc : failed to allocate memory");
        exit(EXIT_FAILURE);
    }
}

static char *find_crlf(const char *buffer, size_t buff_len, size_t start_offset)
{
    if (start_offset >= buff_len)
    {
        return NULL;
    }
    for (size_t i = start_offset; i + 1 < buff_len; i++)
    {
        if (buffer[i] == '\r' && buffer[i+1] == '\n')
        {
            return (char*)&buffer[i];
        }
    }
    return NULL;
}