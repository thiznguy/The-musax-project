#include <stddef.h>
#include <stdint.h>
#include <string.h>

void *memcpy(void *restrict destination, const void *restrict source, size_t count)
{
    uint8_t *to = destination;
    const uint8_t *from = source;

    while (count-- > 0)
        *to++ = *from++;
    return destination;
}

void *memmove(void *destination, const void *source, size_t count)
{
    uint8_t *to = destination;
    const uint8_t *from = source;

    if (to == from)
        return destination;
    if (to < from) {
        while (count-- > 0)
            *to++ = *from++;
    } else {
        while (count-- > 0)
            to[count] = from[count];
    }
    return destination;
}

void *memset(void *destination, int value, size_t count)
{
    uint8_t *to = destination;

    while (count-- > 0)
        *to++ = (uint8_t)value;
    return destination;
}

int memcmp(const void *left, const void *right, size_t count)
{
    const uint8_t *a = left;
    const uint8_t *b = right;

    for (size_t i = 0; i < count; i++) {
        if (a[i] != b[i])
            return (int)a[i] - (int)b[i];
    }
    return 0;
}

void *memchr(const void *region, int value, size_t count)
{
    const uint8_t *bytes = region;

    for (size_t i = 0; i < count; i++) {
        if (bytes[i] == (uint8_t)value)
            return (void *)(bytes + i);
    }
    return NULL;
}

size_t strlen(const char *string)
{
    size_t length = 0;

    while (string[length] != '\0')
        length++;
    return length;
}

size_t strnlen(const char *string, size_t limit)
{
    size_t length = 0;

    while (length < limit && string[length] != '\0')
        length++;
    return length;
}

int strcmp(const char *left, const char *right)
{
    while (*left != '\0' && *left == *right) {
        left++;
        right++;
    }
    return (int)(uint8_t)*left - (int)(uint8_t)*right;
}

int strncmp(const char *left, const char *right, size_t count)
{
    while (count > 0 && *left != '\0' && *left == *right) {
        left++;
        right++;
        count--;
    }
    if (count == 0)
        return 0;
    return (int)(uint8_t)*left - (int)(uint8_t)*right;
}

char *strcpy(char *restrict destination, const char *restrict source)
{
    char *start = destination;

    while ((*destination++ = *source++) != '\0')
        ;
    return start;
}

char *strncpy(char *restrict destination, const char *restrict source, size_t count)
{
    size_t i = 0;

    for (; i < count && source[i] != '\0'; i++)
        destination[i] = source[i];
    for (; i < count; i++)
        destination[i] = '\0';
    return destination;
}

size_t strlcpy(char *restrict destination, const char *restrict source, size_t capacity)
{
    size_t length = strlen(source);

    if (capacity > 0) {
        size_t copied = length < capacity - 1 ? length : capacity - 1;

        memcpy(destination, source, copied);
        destination[copied] = '\0';
    }
    return length;
}

char *strcat(char *restrict destination, const char *restrict source)
{
    strcpy(destination + strlen(destination), source);
    return destination;
}

char *strchr(const char *string, int value)
{
    for (;; string++) {
        if (*string == (char)value)
            return (char *)string;
        if (*string == '\0')
            return NULL;
    }
}

char *strrchr(const char *string, int value)
{
    const char *found = NULL;

    for (;; string++) {
        if (*string == (char)value)
            found = string;
        if (*string == '\0')
            return (char *)found;
    }
}

char *strstr(const char *haystack, const char *needle)
{
    size_t needle_length = strlen(needle);

    if (needle_length == 0)
        return (char *)haystack;
    for (; *haystack != '\0'; haystack++) {
        if (strncmp(haystack, needle, needle_length) == 0)
            return (char *)haystack;
    }
    return NULL;
}
