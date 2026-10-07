#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <musax/arch/x86_64/paging.h>
#include "check.h"

#define ENTRY_ADDRESS_MASK 0x000FFFFFFFFFF000ULL
#define GIB (1024ULL * 1024 * 1024)

/* Walks the tables the way the processor does and returns the physical address, or -1 when the page is not mapped. */
static int64_t translate(const uint64_t *pml4, uint64_t virtual_address)
{
    uint64_t entry = pml4[(virtual_address >> 39) & 0x1FF];
    const uint64_t *table;

    if (!(entry & PAGE_PRESENT))
        return -1;
    table = (const uint64_t *)(uintptr_t)(entry & ENTRY_ADDRESS_MASK);
    entry = table[(virtual_address >> 30) & 0x1FF];
    if (!(entry & PAGE_PRESENT))
        return -1;
    table = (const uint64_t *)(uintptr_t)(entry & ENTRY_ADDRESS_MASK);
    entry = table[(virtual_address >> 21) & 0x1FF];
    if (!(entry & PAGE_PRESENT))
        return -1;
    if (entry & PAGE_HUGE)
        return (int64_t)((entry & ENTRY_ADDRESS_MASK & ~((1ULL << 21) - 1)) | (virtual_address & ((1ULL << 21) - 1)));
    table = (const uint64_t *)(uintptr_t)(entry & ENTRY_ADDRESS_MASK);
    entry = table[(virtual_address >> 12) & 0x1FF];
    if (!(entry & PAGE_PRESENT))
        return -1;
    return (int64_t)((entry & ENTRY_ADDRESS_MASK) | (virtual_address & 0xFFF));
}

int main(void)
{
    uint8_t *region = aligned_alloc(PAGING_PAGE_SIZE, PAGING_TABLE_PAGES * PAGING_PAGE_SIZE);
    const uint64_t *pml4;
    static const uint64_t mapped[] = {
        0x1000, 0x1010, 0xB8000, 0xB8FFF, 0x100000, 0x1FFFFF, 0x200000, 0x200001, 0x7FFFFFF, 0x40000000,
        0x80000000, 0xC0000000, 0xFEE00000, 0xFFFFF000, 0xFFFFFFFF,
    };

    memset(region, 0xAA, PAGING_TABLE_PAGES * PAGING_PAGE_SIZE);
    paging_build_identity(region, (uint64_t)(uintptr_t)region);
    pml4 = (const uint64_t *)region;

    for (size_t i = 0; i < sizeof(mapped) / sizeof(mapped[0]); i++)
        CHECK(translate(pml4, mapped[i]) == (int64_t)mapped[i]);

    for (uint64_t address = 0x1000; address < 2 * 1024 * 1024; address += PAGING_PAGE_SIZE)
        CHECK(translate(pml4, address) == (int64_t)address);
    for (uint64_t address = 2 * 1024 * 1024; address < 4 * GIB; address += 2 * 1024 * 1024 + 4096)
        CHECK(translate(pml4, address) == (int64_t)address);

    CHECK(translate(pml4, 0x0) == -1);
    CHECK(translate(pml4, 0x400) == -1);
    CHECK(translate(pml4, 0xFFF) == -1);
    CHECK(translate(pml4, 4 * GIB) == -1);
    CHECK(translate(pml4, 0x0000800000000000ULL) == -1);
    CHECK(translate(pml4, 1ULL << 39) == -1);

    CHECK(pml4[1] == 0);
    CHECK((pml4[0] & PAGE_WRITABLE) != 0);

    {
        const uint64_t *directory3 = (const uint64_t *)(region + 5 * PAGING_PAGE_SIZE);
        const uint64_t *directory0 = (const uint64_t *)(region + 2 * PAGING_PAGE_SIZE);
        uint64_t before = directory3[(0xFD000000ULL % GIB) / (2 * 1024 * 1024)];

        paging_mark_uncached(region, 0xFD000000ULL, 1024 * 768 * 4);
        for (uint64_t address = 0xFD000000ULL; address < 0xFD000000ULL + 1024 * 768 * 4; address += 4096) {
            uint64_t entry = directory3[(address % GIB) / (2 * 1024 * 1024)];

            CHECK((entry & PAGE_CACHE_DISABLE) != 0 && (entry & PAGE_WRITE_THROUGH) != 0);
            CHECK(translate(pml4, address) == (int64_t)address);
        }
        CHECK((before & PAGE_CACHE_DISABLE) == 0);
        CHECK((directory3[(0xFD000000ULL % GIB) / (2 * 1024 * 1024) + 2 + 1] & PAGE_CACHE_DISABLE) == 0);
        CHECK((directory0[1] & PAGE_CACHE_DISABLE) == 0);

        paging_mark_uncached(region, 0x1000, 0x1000);
        CHECK((directory0[0] & PAGE_CACHE_DISABLE) == 0);
        paging_mark_uncached(region, 0, 0);
        paging_mark_uncached(region, 4 * GIB - 4096, 0x100000);
        CHECK((((const uint64_t *)(region + 5 * PAGING_PAGE_SIZE))[511] & PAGE_CACHE_DISABLE) != 0);
    }

    free(region);
    CHECK_REPORT("paging");
}
