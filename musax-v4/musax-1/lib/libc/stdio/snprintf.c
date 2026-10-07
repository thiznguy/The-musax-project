#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

/* Destination buffer that silently drops characters beyond its capacity. */
struct buffer_sink {
    char *buffer;
    size_t capacity;
    size_t length;
};

static void sink_emit(char c, void *context)
{
    struct buffer_sink *sink = context;

    if (sink->length + 1 < sink->capacity)
        sink->buffer[sink->length] = c;
    sink->length++;
}

int vsnprintf(char *restrict buffer, size_t capacity, const char *restrict format, va_list args)
{
    struct buffer_sink sink = { .buffer = buffer, .capacity = capacity, .length = 0 };
    int count = __vformat(sink_emit, &sink, format, args);

    if (capacity > 0)
        buffer[sink.length < capacity ? sink.length : capacity - 1] = '\0';
    return count;
}

int snprintf(char *restrict buffer, size_t capacity, const char *restrict format, ...)
{
    va_list args;
    int count;

    va_start(args, format);
    count = vsnprintf(buffer, capacity, format, args);
    va_end(args);
    return count;
}
