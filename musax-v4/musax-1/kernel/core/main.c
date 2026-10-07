#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <musax/arch/arch.h>
#include <musax/arch/cpuinfo.h>
#include <musax/arch/x86_64/paging.h>
#include <musax/boot.h>
#include <musax/boot/bootinfo.h>
#include <musax/clock.h>
#include <musax/config.h>
#include <musax/console.h>
#include <musax/drivers/ps2_keyboard.h>
#include <musax/drivers/pci.h>
#include <musax/drivers/timer.h>
#include <musax/fs.h>
#include <musax/kprintf.h>
#include <musax/memmap.h>
#include <musax/net/net.h>
#include <musax/panic.h>
#include <musax/pmm.h>
#include <musax/shell/music.h>
#include <musax/version.h>

#define MIB (1024UL * 1024UL)
#define HEAP_MAX_FRAMES (64ULL * MIB / PMM_FRAME_SIZE)
#define DEFAULT_ANIMATION_DELAY_MS 25

extern char __kernel_end[];

static const struct bootinfo *boot_info;

static int stage_cpu_tables(char *detail, size_t size)
{
    arch_init();
    snprintf(detail, size, "GDT, IDT, TSS");
    return 0;
}

static int stage_cpu(char *detail, size_t size)
{
    struct cpu_info info;

    arch_cpu_describe(&info);
    snprintf(detail, size, "%s", info.brand[0] != '\0' ? info.brand : info.vendor);
    return 0;
}

static int stage_memory_map(char *detail, size_t size)
{
    memmap_init(boot_info);
    snprintf(detail, size, "%lu MiB usable", memmap_usable_bytes() / MIB);
    return 0;
}

static int stage_memory_manager(char *detail, size_t size)
{
    uint64_t kernel_end = ((uint64_t)(uintptr_t)__kernel_end + PMM_FRAME_SIZE - 1) & ~(PMM_FRAME_SIZE - 1);
    const struct bootinfo_memory_entry *entries;
    size_t entry_count;
    uint64_t run_start;
    size_t run_frames;
    size_t heap_frames;
    uint64_t heap;

    entries = memmap_entries(&entry_count);
    pmm_init(entries, entry_count, kernel_end);

    if (!pmm_largest_free_run(&run_start, &run_frames)) {
        snprintf(detail, size, "no free memory");
        return -1;
    }
    heap_frames = run_frames / 2 > HEAP_MAX_FRAMES ? HEAP_MAX_FRAMES : run_frames / 2;
    if (heap_frames == 0)
        heap_frames = run_frames;

    heap = pmm_alloc_contiguous(heap_frames);
    if (heap == 0) {
        snprintf(detail, size, "heap allocation failed");
        return -1;
    }
    __malloc_init((void *)(uintptr_t)heap, heap_frames * PMM_FRAME_SIZE);
    snprintf(detail, size, "%lu MiB heap", (unsigned long)(heap_frames * PMM_FRAME_SIZE / MIB));
    return 0;
}

static int stage_paging(char *detail, size_t size)
{
    if (paging_init() != 0) {
        snprintf(detail, size, "no memory for tables");
        return -1;
    }
    snprintf(detail, size, "4 GiB, null page guarded");
    return 0;
}

static int stage_interrupt_controller(char *detail, size_t size)
{
    arch_init_interrupts();
    snprintf(detail, size, "PIC on vectors 32-47");
    return 0;
}

static int stage_timer(char *detail, size_t size)
{
    bool ticking = timer_start_ticks();
    char rate[16];
    time_t now;
    struct tm date;

    snprintf(rate, sizeof(rate), ticking ? "%u Hz" : "polling", TIMER_TICK_HZ);
    if (clock_init() && clock_now(&now)) {
        gmtime_r(&now, &date);
        snprintf(detail, size, "%s, %04d-%02d-%02d", rate, date.tm_year + 1900, date.tm_mon + 1, date.tm_mday);
    } else {
        snprintf(detail, size, "%s, no clock", rate);
    }
    return 0;
}

static int stage_filesystem(char *detail, size_t size)
{
    size_t length;
    const char *text = config_default_text(&length);
    int error;

    fs_init();
    error = fs_write_file(CONFIG_FILE_PATH, text, length);
    if (error != FS_OK) {
        snprintf(detail, size, "%s", fs_error_message(error));
        return -1;
    }
    snprintf(detail, size, "in-memory, 8 directories");
    return 0;
}

static int stage_configuration(char *detail, size_t size)
{
    int applied = config_reload();

    snprintf(detail, size, applied >= 0 ? "%d settings" : "defaults", applied);
    return 0;
}

static int stage_network(char *detail, size_t size)
{
    char address[24];

    pci_init();
    if (!net_init()) {
        snprintf(detail, size, "no network adapter");
        return 0;
    }
    if (!config_get_bool("net.dhcp", true)) {
        snprintf(detail, size, "%s, no address", net_device()->name);
        return 0;
    }
    if (!net_dhcp()) {
        snprintf(detail, size, "no DHCP answer");
        return 0;
    }
    net_format_ip(net_get_config()->ip, address, sizeof(address));
    snprintf(detail, size, "%s", address);
    return 0;
}

static int stage_shell(char *detail, size_t size)
{
    snprintf(detail, size, "ready");
    return 0;
}

static const struct boot_stage boot_stages[] = {
    { "Installing CPU tables", stage_cpu_tables },
    { "Detecting CPU", stage_cpu },
    { "Reading memory map", stage_memory_map },
    { "Starting memory manager", stage_memory_manager },
    { "Enabling paging", stage_paging },
    { "Remapping interrupt controller", stage_interrupt_controller },
    { "Starting timer and clock", stage_timer },
    { "Mounting filesystem", stage_filesystem },
    { "Loading configuration", stage_configuration },
    { "Starting network", stage_network },
    { "Starting muSIC", stage_shell },
};

void kmain(const struct bootinfo *info)
{
    boot_info = info;
    bootinfo_set(info);

    console_init();
    ps2_keyboard_init();
    timer_init();
    config_load_defaults();

    boot_run(boot_stages, sizeof(boot_stages) / sizeof(boot_stages[0]), config_get_bool("boot.animation", true),
             (unsigned)config_get_long("boot.animation_delay_ms", DEFAULT_ANIMATION_DELAY_MS));

    kprintf("%s %s\n", MUSAX_NAME, MUSAX_VERSION);
    kprintf("%lu MiB usable memory. Type help for the list of commands.\n\n", memmap_usable_bytes() / MIB);

    music_run();
}
