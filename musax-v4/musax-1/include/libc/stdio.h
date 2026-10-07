#ifndef LIBC_STDIO_H
#define LIBC_STDIO_H

#include <stdarg.h>
#include <stddef.h>

int snprintf(char *restrict buffer, size_t capacity, const char *restrict format, ...)
    __attribute__((format(printf, 3, 4)));
int vsnprintf(char *restrict buffer, size_t capacity, const char *restrict format, va_list args)
    __attribute__((format(printf, 3, 0)));

/* Formatting engine shared by vsnprintf and the kernel console: each output character goes to emit. */
int __vformat(void (*emit)(char c, void *context), void *context, const char *format, va_list args)
    __attribute__((format(printf, 3, 0)));

#endif
