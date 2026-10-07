#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define ALIGNMENT 16
#define MIN_SPLIT_PAYLOAD 16

/* Boundary header in front of every allocation; blocks tile the arena contiguously in address order. */
struct block {
    size_t size;
    size_t is_free;
    struct block *prev;
    struct block *next;
};

#define HEADER_SIZE sizeof(struct block)

static struct block *first_block;

static size_t align_up(size_t value)
{
    return (value + ALIGNMENT - 1) & ~(size_t)(ALIGNMENT - 1);
}

static void *payload_of(struct block *block)
{
    return (uint8_t *)block + HEADER_SIZE;
}

static struct block *block_of(void *payload)
{
    return (struct block *)((uint8_t *)payload - HEADER_SIZE);
}

static void merge_with_next(struct block *block)
{
    struct block *next = block->next;

    if (next == NULL || !next->is_free)
        return;
    block->size += HEADER_SIZE + next->size;
    block->next = next->next;
    if (block->next != NULL)
        block->next->prev = block;
}

static void split(struct block *block, size_t size)
{
    struct block *rest;

    if (block->size < size + HEADER_SIZE + MIN_SPLIT_PAYLOAD)
        return;
    rest = (struct block *)((uint8_t *)block + HEADER_SIZE + size);
    rest->size = block->size - size - HEADER_SIZE;
    rest->is_free = 1;
    rest->prev = block;
    rest->next = block->next;
    if (block->next != NULL)
        block->next->prev = rest;
    block->next = rest;
    block->size = size;
    merge_with_next(rest);
}

void __malloc_init(void *base, size_t size)
{
    uintptr_t start = ((uintptr_t)base + ALIGNMENT - 1) & ~(uintptr_t)(ALIGNMENT - 1);
    uintptr_t end = ((uintptr_t)base + size) & ~(uintptr_t)(ALIGNMENT - 1);

    first_block = NULL;
    if (end <= start || end - start < HEADER_SIZE + ALIGNMENT)
        return;

    first_block = (struct block *)start;
    first_block->size = end - start - HEADER_SIZE;
    first_block->is_free = 1;
    first_block->prev = NULL;
    first_block->next = NULL;
}

void *malloc(size_t size)
{
    if (size == 0)
        size = 1;
    if (size > SIZE_MAX - ALIGNMENT - HEADER_SIZE)
        return NULL;
    size = align_up(size);

    for (struct block *block = first_block; block != NULL; block = block->next) {
        if (block->is_free && block->size >= size) {
            split(block, size);
            block->is_free = 0;
            return payload_of(block);
        }
    }
    return NULL;
}

void free(void *pointer)
{
    struct block *block;

    if (pointer == NULL)
        return;
    block = block_of(pointer);
    block->is_free = 1;
    merge_with_next(block);
    if (block->prev != NULL && block->prev->is_free)
        merge_with_next(block->prev);
}

void *calloc(size_t count, size_t size)
{
    void *pointer;

    if (size != 0 && count > SIZE_MAX / size)
        return NULL;
    pointer = malloc(count * size);
    if (pointer != NULL)
        memset(pointer, 0, count * size);
    return pointer;
}

void *realloc(void *pointer, size_t size)
{
    struct block *block;
    void *moved;

    if (pointer == NULL)
        return malloc(size);
    if (size == 0) {
        free(pointer);
        return NULL;
    }
    if (size > SIZE_MAX - ALIGNMENT - HEADER_SIZE)
        return NULL;

    block = block_of(pointer);
    size = align_up(size);

    if (block->size >= size) {
        split(block, size);
        return pointer;
    }
    if (block->next != NULL && block->next->is_free &&
        block->size + HEADER_SIZE + block->next->size >= size) {
        merge_with_next(block);
        split(block, size);
        return pointer;
    }

    moved = malloc(size);
    if (moved == NULL)
        return NULL;
    memcpy(moved, pointer, block->size);
    free(pointer);
    return moved;
}

void __malloc_get_info(struct __malloc_info *info)
{
    memset(info, 0, sizeof(*info));
    for (struct block *block = first_block; block != NULL; block = block->next) {
        info->arena_bytes += HEADER_SIZE + block->size;
        info->block_count++;
        if (block->is_free) {
            info->free_bytes += block->size;
            if (block->size > info->largest_free_block)
                info->largest_free_block = block->size;
        } else {
            info->used_bytes += block->size;
        }
    }
}
