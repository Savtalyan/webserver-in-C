#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>

static void safe_strncpy(char *dest, const char *src, size_t dest_size);

static void malloc_check(void *ptr);

static char *find_crlf(const char *buffer, size_t buff_len, size_t start_offset);

#endif 