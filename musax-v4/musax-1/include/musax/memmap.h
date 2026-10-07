#ifndef MUSAX_MEMMAP_H
#define MUSAX_MEMMAP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <musax/boot/bootinfo.h>

void memmap_init(const struct bootinfo *info);
uint64_t memmap_usable_bytes(void);
void memmap_print(void);

/* Raw access to the firmware entries for the frame allocator. */
const struct bootinfo_memory_entry *memmap_entries(size_t *count);

/* Finds the largest usable region inside [min_base, max_end), aligned to 4 KiB pages. */
bool memmap_find_largest_usable(uint64_t min_base, uint64_t max_end, uint64_t *base, uint64_t *length);

#endif
