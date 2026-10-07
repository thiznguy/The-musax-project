#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <musax/arch/arch.h>
#include <musax/arch/cpu.h>
#include <musax/arch/cpuinfo.h>
#include <musax/boot.h>
#include <musax/clock.h>
#include <musax/config.h>
#include <musax/console.h>
#include <musax/drivers/timer.h>
#include <musax/kprintf.h>
#include <musax/memmap.h>
#include <musax/pmm.h>
#include <musax/version.h>
#include "../internal.h"

#define MAX_SLEEP_SECONDS 3600
#define MILLISECONDS_PER_SECOND 1000
#define DEFAULT_HOSTNAME "musax"

static const char *const weekday_names[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

int command_help(int argc, char **argv)
{
    if (argc > 1) {
        for (size_t i = 0; i < music_command_count; i++) {
            if (strcmp(argv[1], music_commands[i].name) == 0) {
                kprintf("usage: %s\n%s\n", music_commands[i].usage, music_commands[i].summary);
                return 0;
            }
        }
        kprintf("help: no such command: %s\n", argv[1]);
        return 1;
    }

    for (size_t i = 0; i < music_command_count; i++)
        kprintf("%-9s %s\n", music_commands[i].name, music_commands[i].summary);
    return 0;
}

int command_version(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    kprintf("%s %s\n", MUSAX_NAME, MUSAX_VERSION);
    return 0;
}

int command_echo(int argc, char **argv)
{
    for (int i = 1; i < argc; i++) {
        if (i > 1)
            console_putc(' ');
        console_puts(argv[i]);
    }
    console_putc('\n');
    return 0;
}

int command_clear(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    console_clear();
    return 0;
}

int command_reboot(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    arch_reboot();
}

int command_halt(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    kprintf("System halted.\n");
    cpu_halt_forever();
}

int command_memmap(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    memmap_print();
    return 0;
}

int command_free(int argc, char **argv)
{
    struct __malloc_info info;

    (void)argc;
    (void)argv;

    __malloc_get_info(&info);
    kprintf("heap: %zu KiB total, %zu KiB used, %zu KiB free, largest free block %zu KiB\n", info.arena_bytes / 1024,
            info.used_bytes / 1024, info.free_bytes / 1024, info.largest_free_block / 1024);
    return 0;
}

int command_date(int argc, char **argv)
{
    time_t now;
    struct tm broken_down;

    (void)argc;
    (void)argv;

    if (!clock_now(&now)) {
        kprintf("date: real-time clock unavailable\n");
        return 1;
    }
    gmtime_r(&now, &broken_down);
    kprintf("%s %04d-%02d-%02d %02d:%02d:%02d\n", weekday_names[broken_down.tm_wday], broken_down.tm_year + 1900,
            broken_down.tm_mon + 1, broken_down.tm_mday, broken_down.tm_hour, broken_down.tm_min,
            broken_down.tm_sec);
    return 0;
}

int command_uptime(int argc, char **argv)
{
    time_t seconds;

    (void)argc;
    (void)argv;

    if (timer_ticks_running()) {
        seconds = (time_t)(timer_uptime_ms() / MILLISECONDS_PER_SECOND);
    } else if (!clock_uptime(&seconds)) {
        kprintf("uptime: no timer or real-time clock available\n");
        return 1;
    }
    kprintf("up %lldh %02lldm %02llds\n", seconds / 3600, (seconds / 60) % 60, seconds % 60);
    return 0;
}

int command_sleep(int argc, char **argv)
{
    char *end;
    long seconds;

    if (argc != 2) {
        kprintf("usage: sleep seconds\n");
        return 1;
    }
    seconds = strtol(argv[1], &end, 10);
    if (*end != '\0' || seconds < 0 || seconds > MAX_SLEEP_SECONDS) {
        kprintf("sleep: seconds must be a number from 0 to %d\n", MAX_SLEEP_SECONDS);
        return 1;
    }
    for (long i = 0; i < seconds; i++)
        timer_delay_ms(MILLISECONDS_PER_SECOND);
    return 0;
}

int command_hostname(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    kprintf("%s\n", config_get("system.hostname", DEFAULT_HOSTNAME));
    return 0;
}

int command_bootanim(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    if (console_is_graphical()) {
        kprintf("bootanim: needs the text console; leave the desktop first\n");
        return 1;
    }

    boot_splash_demo((unsigned)config_get_long("boot.animation_delay_ms", 25));
    return 0;
}

int command_mem(int argc, char **argv)
{
    struct pmm_stats stats;
    unsigned long frame_kib = PMM_FRAME_SIZE / 1024;

    (void)argc;
    (void)argv;

    pmm_get_stats(&stats);
    kprintf("frames: %zu total, %zu free, %zu in use (%lu KiB each)\n", stats.total_frames, stats.free_frames,
            stats.total_frames - stats.free_frames, frame_kib);
    kprintf("free memory: %lu KiB\n", stats.free_frames * frame_kib);
    return 0;
}

int command_cpuinfo(int argc, char **argv)
{
    struct cpu_info info;

    (void)argc;
    (void)argv;

    arch_cpu_describe(&info);
    kprintf("vendor:   %s\n", info.vendor);
    kprintf("model:    %s\n", info.brand[0] != '\0' ? info.brand : "not reported");
    kprintf("family %u, model %u, stepping %u\n", info.family, info.model, info.stepping);
    return 0;
}
