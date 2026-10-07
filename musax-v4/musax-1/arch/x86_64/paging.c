#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <musax/arch/x86_64/paging.h>
#include <musax/pmm.h>

#define ENTRIES_PER_TABLE 512
#define PDPT_ENTRIES_USED 4
#define HUGE_PAGE_SIZE (2ULL * 1024 * 1024)
#define GIGABYTE (1024ULL * 1024 * 1024)

enum {
    PAGE_PML4 = 0,
    PAGE_PDPT = 1,
    PAGE_PD_FIRST = 2,
    PAGE_PT_FIRST_2MB = 6,
};

void paging_build_identity(void *tables, uint64_t tables_physical)
{
    uint64_t *pml4 = tables;
    uint64_t *pdpt = (uint64_t *)((uint8_t *)tables + PAGE_PDPT * PAGING_PAGE_SIZE);
    uint64_t *first_table = (uint64_t *)((uint8_t *)tables + PAGE_PT_FIRST_2MB * PAGING_PAGE_SIZE);

    memset(tables, 0, PAGING_TABLE_PAGES * PAGING_PAGE_SIZE);

    pml4[0] = (tables_physical + PAGE_PDPT * PAGING_PAGE_SIZE) | PAGE_PRESENT | PAGE_WRITABLE;

    for (unsigned i = 0; i < PDPT_ENTRIES_USED; i++) {
        uint64_t *directory = (uint64_t *)((uint8_t *)tables + (PAGE_PD_FIRST + i) * PAGING_PAGE_SIZE);
        uint64_t directory_physical = tables_physical + (PAGE_PD_FIRST + i) * PAGING_PAGE_SIZE;

        pdpt[i] = directory_physical | PAGE_PRESENT | PAGE_WRITABLE;
        for (unsigned j = 0; j < ENTRIES_PER_TABLE; j++)
            directory[j] = (i * GIGABYTE + j * HUGE_PAGE_SIZE) | PAGE_PRESENT | PAGE_WRITABLE | PAGE_HUGE;
    }

    ((uint64_t *)((uint8_t *)tables + PAGE_PD_FIRST * PAGING_PAGE_SIZE))[0] =
        (tables_physical + PAGE_PT_FIRST_2MB * PAGING_PAGE_SIZE) | PAGE_PRESENT | PAGE_WRITABLE;

    for (unsigned k = 1; k < ENTRIES_PER_TABLE; k++)
        first_table[k] = (k * PAGING_PAGE_SIZE) | PAGE_PRESENT | PAGE_WRITABLE;
}

void paging_mark_uncached(void *tables, uint64_t start, uint64_t length)
{
    uint64_t end = start + length;
    uint64_t address;

    if (length == 0 || start < HUGE_PAGE_SIZE)
        return;
    if (end > PDPT_ENTRIES_USED * GIGABYTE)
        end = PDPT_ENTRIES_USED * GIGABYTE;

    for (address = start & ~(HUGE_PAGE_SIZE - 1); address < end; address += HUGE_PAGE_SIZE) {
        uint64_t *directory = (uint64_t *)((uint8_t *)tables +
                                            (PAGE_PD_FIRST + address / GIGABYTE) * PAGING_PAGE_SIZE);

        directory[(address % GIGABYTE) / HUGE_PAGE_SIZE] |= PAGE_WRITE_THROUGH | PAGE_CACHE_DISABLE;
    }
}

static void *active_tables;

void paging_set_uncached(uint64_t start, uint64_t length)
{
    uint64_t cr3;

    if (active_tables == NULL)
        return;
    paging_mark_uncached(active_tables, start, length);
    __asm__ volatile("mov %%cr3, %0\n\tmov %0, %%cr3" : "=r"(cr3) : : "memory");
}

int paging_init(void)
{
    uint64_t physical = pmm_alloc_contiguous(PAGING_TABLE_PAGES);

    if (physical == 0)
        return -1;
    paging_build_identity((void *)(uintptr_t)physical, physical);
    active_tables = (void *)(uintptr_t)physical;
    __asm__ volatile("mov %0, %%cr3" : : "r"(physical) : "memory");
    return 0;
}
