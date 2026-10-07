#ifndef MUSAX_BOOT_BOOTINFO_H
#define MUSAX_BOOT_BOOTINFO_H

#include <stdint.h>

#define BOOTINFO_MAGIC 0x5853554DU
#define BOOTINFO_MAX_MEMORY_ENTRIES 64

#define MEMORY_TYPE_USABLE 1
#define MEMORY_TYPE_RESERVED 2
#define MEMORY_TYPE_ACPI_RECLAIMABLE 3
#define MEMORY_TYPE_ACPI_NVS 4
#define MEMORY_TYPE_BAD 5

/* One BIOS E820 memory map entry. */
struct bootinfo_memory_entry {
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t acpi_attributes;
} __attribute__((packed));

/* Boot information block written by stage 2 and passed to the kernel. */
struct bootinfo {
    uint32_t magic;
    uint32_t memory_entry_count;
    uint32_t boot_drive;
    uint32_t font_address;
    struct bootinfo_memory_entry memory[BOOTINFO_MAX_MEMORY_ENTRIES];
} __attribute__((packed));

/* Remembers the boot information block; bootinfo_font returns the 8x16 BIOS font copied by stage 2, or NULL. */
void bootinfo_set(const struct bootinfo *info);
const uint8_t *bootinfo_font(void);

#endif
