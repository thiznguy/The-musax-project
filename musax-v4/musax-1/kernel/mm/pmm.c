#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <musax/pmm.h>

/* One bit per 4 KiB frame of the first 4 GiB; a set bit means the frame is in use. */
static uint8_t used_bitmap[PMM_MAX_FRAMES / 8];
static size_t total_frames;
static size_t free_frames;
static size_t search_start;

static int is_used(size_t frame)
{
    return (used_bitmap[frame / 8] >> (frame % 8)) & 1;
}

static void set_used(size_t frame)
{
    used_bitmap[frame / 8] |= (uint8_t)(1U << (frame % 8));
}

static void set_free(size_t frame)
{
    used_bitmap[frame / 8] &= (uint8_t)~(1U << (frame % 8));
}

void pmm_init(const struct bootinfo_memory_entry *entries, size_t count, uint64_t reserved_end)
{
    memset(used_bitmap, 0xFF, sizeof(used_bitmap));
    total_frames = 0;
    free_frames = 0;
    search_start = 0;

    for (size_t i = 0; i < count; i++) {
        uint64_t start;
        uint64_t end;

        if (entries[i].type != MEMORY_TYPE_USABLE)
            continue;

        start = entries[i].base > reserved_end ? entries[i].base : reserved_end;
        end = entries[i].base + entries[i].length;
        if (end > PMM_MAX_FRAMES * PMM_FRAME_SIZE)
            end = PMM_MAX_FRAMES * PMM_FRAME_SIZE;

        start = (start + PMM_FRAME_SIZE - 1) / PMM_FRAME_SIZE;
        end /= PMM_FRAME_SIZE;

        for (uint64_t frame = start; frame < end; frame++) {
            if (is_used((size_t)frame)) {
                set_free((size_t)frame);
                total_frames++;
                free_frames++;
            }
        }
    }
}

uint64_t pmm_alloc_contiguous(size_t count)
{
    size_t run = 0;

    if (count == 0 || count > free_frames)
        return 0;

    for (size_t frame = 0; frame < PMM_MAX_FRAMES; frame++) {
        if (is_used(frame)) {
            run = 0;
            continue;
        }
        if (++run == count) {
            size_t first = frame + 1 - count;

            for (size_t i = first; i <= frame; i++)
                set_used(i);
            free_frames -= count;
            return (uint64_t)first * PMM_FRAME_SIZE;
        }
    }
    return 0;
}

uint64_t pmm_alloc_frame(void)
{
    for (size_t offset = 0; offset < PMM_MAX_FRAMES; offset++) {
        size_t frame = (search_start + offset) % PMM_MAX_FRAMES;

        if (!is_used(frame)) {
            set_used(frame);
            free_frames--;
            search_start = frame + 1;
            return (uint64_t)frame * PMM_FRAME_SIZE;
        }
    }
    return 0;
}

void pmm_free_frames(uint64_t physical, size_t count)
{
    size_t first = (size_t)(physical / PMM_FRAME_SIZE);

    for (size_t i = 0; i < count && first + i < PMM_MAX_FRAMES; i++) {
        if (is_used(first + i)) {
            set_free(first + i);
            free_frames++;
        }
    }
    if (first < search_start)
        search_start = first;
}

int pmm_largest_free_run(uint64_t *physical, size_t *count)
{
    size_t best_start = 0;
    size_t best_length = 0;
    size_t run_start = 0;
    size_t run_length = 0;

    for (size_t frame = 0; frame < PMM_MAX_FRAMES; frame++) {
        if (is_used(frame)) {
            run_length = 0;
            continue;
        }
        if (run_length == 0)
            run_start = frame;
        if (++run_length > best_length) {
            best_length = run_length;
            best_start = run_start;
        }
    }

    if (best_length == 0)
        return 0;
    *physical = (uint64_t)best_start * PMM_FRAME_SIZE;
    *count = best_length;
    return 1;
}

void pmm_get_stats(struct pmm_stats *stats)
{
    stats->total_frames = total_frames;
    stats->free_frames = free_frames;
}
