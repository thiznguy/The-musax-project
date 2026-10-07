#include <string.h>
#include <musax/pmm.h>
#include "check.h"

#define MIB (1024ULL * 1024)
#define GIB (1024ULL * MIB)
#define RESERVED_END 0x110000ULL

static struct bootinfo_memory_entry entries[] = {
    { 0x0, 0x9F000, MEMORY_TYPE_USABLE, 1 },
    { 0x9F000, 0x61000, MEMORY_TYPE_RESERVED, 1 },
    { 0x100000, 63 * MIB, MEMORY_TYPE_USABLE, 1 },
    { 0xFEE00000, 0x1000, MEMORY_TYPE_RESERVED, 1 },
    { 4 * GIB, 1 * GIB, MEMORY_TYPE_USABLE, 1 },
};

static size_t expected_free_frames(void)
{
    return (size_t)((64 * MIB - RESERVED_END) / PMM_FRAME_SIZE);
}

static void test_init(void)
{
    struct pmm_stats stats;
    uint64_t start;
    size_t count;

    pmm_init(entries, sizeof(entries) / sizeof(entries[0]), RESERVED_END);
    pmm_get_stats(&stats);
    CHECK(stats.total_frames == expected_free_frames());
    CHECK(stats.free_frames == stats.total_frames);

    CHECK(pmm_largest_free_run(&start, &count) == 1);
    CHECK(start == RESERVED_END);
    CHECK(count == expected_free_frames());
}

static void test_single_frames(void)
{
    struct pmm_stats before;
    struct pmm_stats after;
    uint64_t first;
    uint64_t second;

    pmm_get_stats(&before);
    first = pmm_alloc_frame();
    second = pmm_alloc_frame();
    CHECK(first >= RESERVED_END && first % PMM_FRAME_SIZE == 0);
    CHECK(second == first + PMM_FRAME_SIZE);

    pmm_get_stats(&after);
    CHECK(after.free_frames == before.free_frames - 2);

    pmm_free_frames(first, 1);
    CHECK(pmm_alloc_frame() == first);
    pmm_free_frames(first, 2);
    pmm_get_stats(&after);
    CHECK(after.free_frames == before.free_frames);

    pmm_free_frames(first, 1);
    pmm_get_stats(&after);
    CHECK(after.free_frames == before.free_frames);
}

static void test_contiguous_and_fragmentation(void)
{
    uint64_t run = pmm_alloc_contiguous(100);
    uint64_t other;
    uint64_t largest_start;
    size_t largest_count;
    struct pmm_stats stats;

    CHECK(run >= RESERVED_END && run % PMM_FRAME_SIZE == 0);
    other = pmm_alloc_contiguous(10);
    CHECK(other == run + 100 * PMM_FRAME_SIZE);

    pmm_free_frames(run, 100);
    CHECK(pmm_alloc_contiguous(100) == run);
    CHECK(pmm_alloc_contiguous(101) != run);

    pmm_get_stats(&stats);
    CHECK(pmm_largest_free_run(&largest_start, &largest_count) == 1);
    CHECK(largest_count <= stats.free_frames);
    CHECK(largest_start > other);

    CHECK(pmm_alloc_contiguous(0) == 0);
    CHECK(pmm_alloc_contiguous(stats.free_frames + 1) == 0);
}

static void test_exhaustion(void)
{
    struct pmm_stats stats;
    size_t taken = 0;

    pmm_init(entries, sizeof(entries) / sizeof(entries[0]), RESERVED_END);
    pmm_get_stats(&stats);

    while (pmm_alloc_frame() != 0)
        taken++;
    CHECK(taken == stats.total_frames);
    pmm_get_stats(&stats);
    CHECK(stats.free_frames == 0);
    CHECK(pmm_alloc_contiguous(1) == 0);
    CHECK(pmm_largest_free_run(&(uint64_t){ 0 }, &(size_t){ 0 }) == 0);

    pmm_free_frames(RESERVED_END, 3);
    pmm_get_stats(&stats);
    CHECK(stats.free_frames == 3);
}

static void test_memory_above_4gib_is_ignored(void)
{
    struct pmm_stats stats;
    uint64_t frame;

    pmm_init(entries, sizeof(entries) / sizeof(entries[0]), RESERVED_END);
    pmm_get_stats(&stats);
    CHECK(stats.total_frames < (size_t)(5 * GIB / PMM_FRAME_SIZE));

    while ((frame = pmm_alloc_frame()) != 0)
        CHECK(frame < 4 * GIB);
}

int main(void)
{
    test_init();
    test_single_frames();
    test_contiguous_and_fragmentation();
    test_exhaustion();
    test_memory_above_4gib_is_ignored();
    CHECK_REPORT("pmm");
}
