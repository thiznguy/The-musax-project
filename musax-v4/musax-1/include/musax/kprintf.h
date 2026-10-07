#ifndef MUSAX_KPRINTF_H
#define MUSAX_KPRINTF_H

#include <stdarg.h>

int kprintf(const char *format, ...) __attribute__((format(printf, 1, 2)));
int kvprintf(const char *format, va_list args) __attribute__((format(printf, 1, 0)));

#endif
