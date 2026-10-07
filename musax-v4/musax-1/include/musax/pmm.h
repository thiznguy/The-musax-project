#ifndef MUSAX_PMM_H
#define MUSAX_PMM_H

#include <stddef.h>
#include <stdint.h>
#include <musax/boot/bootinfo.h>

#define PMM_FRAME_SIZE 4096ULL
#define PMM_MAX_FRAMES (1ULL << 20)

struct pmm_stats {
    size_t total_frames;
    size_t free_frames;
};

/* Builds the frame bitmap from the firmware memory map. Frames below reserved_end and frames at or above 4 GiB stay unavailable. */
void pmm_init(const struct bootinfo_memory_entry *entries, size_t count, uint64_t reserved_end);

/* Both allocators return the physical address of the first frame, or 0 when no suitable frames are free. */
uint64_t pmm_alloc_frame(void);
uint64_t pmm_alloc_contiguous(size_t count);
void pmm_free_frames(uint64_t physical, size_t count);

/* Finds the longest run of free frames; returns false when none are free. */
int pmm_largest_free_run(uint64_t *physical, size_t *count);
void pmm_get_stats(struct pmm_stats *stats);

#endif
