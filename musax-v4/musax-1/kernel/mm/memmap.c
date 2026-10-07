#include <stdbool.h>
#include <stdint.h>
#include <musax/kprintf.h>
#include <musax/memmap.h>
#include <musax/panic.h>

static const struct bootinfo_memory_entry *entries;
static uint32_t entry_count;

static const char *type_name(uint32_t type)
{
    switch (type) {
    case MEMORY_TYPE_USABLE:
        return "usable";
    case MEMORY_TYPE_RESERVED:
        return "reserved";
    case MEMORY_TYPE_ACPI_RECLAIMABLE:
        return "acpi reclaimable";
    case MEMORY_TYPE_ACPI_NVS:
        return "acpi nvs";
    case MEMORY_TYPE_BAD:
        return "bad";
    default:
        return "unknown";
    }
}

void memmap_init(const struct bootinfo *info)
{
    if (info->magic != BOOTINFO_MAGIC)
        kpanic("boot information block has bad magic %08x", info->magic);
    if (info->memory_entry_count == 0 || info->memory_entry_count > BOOTINFO_MAX_MEMORY_ENTRIES)
        kpanic("boot information block has %u memory entries", info->memory_entry_count);

    entries = info->memory;
    entry_count = info->memory_entry_count;
}

uint64_t memmap_usable_bytes(void)
{
    uint64_t total = 0;

    for (uint32_t i = 0; i < entry_count; i++) {
        if (entries[i].type == MEMORY_TYPE_USABLE)
            total += entries[i].length;
    }
    return total;
}

void memmap_print(void)
{
    for (uint32_t i = 0; i < entry_count; i++) {
        kprintf("%016lx-%016lx  %s\n", entries[i].base, entries[i].base + entries[i].length - 1,
                type_name(entries[i].type));
    }
}

bool memmap_find_largest_usable(uint64_t min_base, uint64_t max_end, uint64_t *base, uint64_t *length)
{
    const uint64_t page = 4096;
    uint64_t best_base = 0;
    uint64_t best_length = 0;

    for (uint32_t i = 0; i < entry_count; i++) {
        uint64_t start;
        uint64_t end;

        if (entries[i].type != MEMORY_TYPE_USABLE)
            continue;

        start = entries[i].base > min_base ? entries[i].base : min_base;
        end = entries[i].base + entries[i].length;
        if (end > max_end)
            end = max_end;

        start = (start + page - 1) & ~(page - 1);
        end &= ~(page - 1);
        if (end <= start)
            continue;
        if (end - start > best_length) {
            best_base = start;
            best_length = end - start;
        }
    }

    if (best_length == 0)
        return false;
    *base = best_base;
    *length = best_length;
    return true;
}

const struct bootinfo_memory_entry *memmap_entries(size_t *count)
{
    *count = entry_count;
    return entries;
}
