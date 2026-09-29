#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>

// copies at most dest_size-1 bytes, always NUL-terminates
void safe_strncpy(char *dest, const char *src, size_t dest_size);

// exits the process if ptr is NULL
void malloc_check(void *ptr);

// finds "\r\n" in buffer starting at start_offset; NULL if not found
const char *find_crlf(const char *buffer, size_t buff_len, size_t start_offset);

#endif