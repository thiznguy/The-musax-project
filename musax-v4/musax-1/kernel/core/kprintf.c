#include <stdarg.h>
#include <stdio.h>
#include <musax/console.h>
#include <musax/kprintf.h>

static void console_emit(char c, void *context)
{
    (void)context;
    console_putc(c);
}

int kvprintf(const char *format, va_list args)
{
    return __vformat(console_emit, NULL, format, args);
}

int kprintf(const char *format, ...)
{
    va_list args;
    int written;

    va_start(args, format);
    written = kvprintf(format, args);
    va_end(args);
    return written;
}
