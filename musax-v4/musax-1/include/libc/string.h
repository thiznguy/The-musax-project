#ifndef LIBC_STRING_H
#define LIBC_STRING_H

#include <stddef.h>

void *memcpy(void *restrict destination, const void *restrict source, size_t count);
void *memmove(void *destination, const void *source, size_t count);
void *memset(void *destination, int value, size_t count);
int memcmp(const void *left, const void *right, size_t count);
void *memchr(const void *region, int value, size_t count);
size_t strlen(const char *string);
size_t strnlen(const char *string, size_t limit);
int strcmp(const char *left, const char *right);
int strncmp(const char *left, const char *right, size_t count);
char *strcpy(char *restrict destination, const char *restrict source);
char *strncpy(char *restrict destination, const char *restrict source, size_t count);
size_t strlcpy(char *restrict destination, const char *restrict source, size_t capacity);
char *strcat(char *restrict destination, const char *restrict source);
char *strchr(const char *string, int value);
char *strrchr(const char *string, int value);
char *strstr(const char *haystack, const char *needle);

#endif
