#ifndef MUSAX_ARCH_X86_64_PAGING_H
#define MUSAX_ARCH_X86_64_PAGING_H

#include <stddef.h>
#include <stdint.h>

#define PAGING_PAGE_SIZE 4096ULL

/* Table pages needed by the kernel mapping: PML4, PDPT, four page directories, and one page table for the first 2 MiB. */
#define PAGING_TABLE_PAGES 7

#define PAGE_PRESENT 0x1ULL
#define PAGE_WRITABLE 0x2ULL
#define PAGE_HUGE 0x80ULL

/* Fills PAGING_TABLE_PAGES zeroed-then-written pages at tables with an identity map of the first 4 GiB.
   2 MiB pages cover everything except the first 2 MiB, which uses 4 KiB pages with page 0 left unmapped so null pointers fault. */
void paging_build_identity(void *tables, uint64_t tables_physical);

#define PAGE_WRITE_THROUGH 0x8ULL
#define PAGE_CACHE_DISABLE 0x10ULL

/* Marks the 2 MiB pages that overlap [start, start + length) as uncached in tables built by paging_build_identity.
   Ranges inside the first 2 MiB and above 4 GiB are left alone. */
void paging_mark_uncached(void *tables, uint64_t start, uint64_t length);

/* paging_mark_uncached on the tables the kernel is running with, followed by a TLB flush. */
void paging_set_uncached(uint64_t start, uint64_t length);

/* Allocates the table pages from the frame allocator, builds the mapping and loads it into CR3. Returns 0 on success. */
int paging_init(void);

#endif
