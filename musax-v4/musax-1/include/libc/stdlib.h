#ifndef LIBC_STDLIB_H
#define LIBC_STDLIB_H

#include <stddef.h>

/* Usage counters of the allocator arena. */
struct __malloc_info {
    size_t arena_bytes;
    size_t used_bytes;
    size_t free_bytes;
    size_t largest_free_block;
    size_t block_count;
};

void *malloc(size_t size);
void *calloc(size_t count, size_t size);
void *realloc(void *pointer, size_t size);
void free(void *pointer);

/* Hands the allocator the memory region it manages; called once by the kernel before the first allocation. */
void __malloc_init(void *base, size_t size);
void __malloc_get_info(struct __malloc_info *info);

long strtol(const char *string, char **end, int base);
unsigned long strtoul(const char *string, char **end, int base);
int atoi(const char *string);
int abs(int value);
long labs(long value);

#endif
