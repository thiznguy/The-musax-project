#include <stdint.h>
#include <string.h>
#include <musax/drivers/fault_injection.h>
#include <musax/kprintf.h>
#include <musax/panic.h>
#include "../internal.h"

#define VECTOR_MACHINE_CHECK_TEXT "18"

static void kernel_null_write(void)
{
    volatile uintptr_t address = 0;

    *(volatile uint32_t *)address = 0xDEAD;
}

int command_crashtest(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "driver") == 0) {
        fault_inject_driver_null_write();
    } else if (argc == 2 && strcmp(argv[1], "cpu") == 0) {
        __asm__ volatile("int $" VECTOR_MACHINE_CHECK_TEXT);
    } else if (argc == 2 && strcmp(argv[1], "fault") == 0) {
        kernel_null_write();
    } else if (argc == 2 && strcmp(argv[1], "panic") == 0) {
        kpanic("crashtest panic requested from the shell");
    }

    kprintf("usage: crashtest driver|cpu|fault|panic\n");
    kprintf("  driver  null write inside driver code      -> driver error code\n");
    kprintf("  cpu     machine check exception            -> CPU/motherboard error code\n");
    kprintf("  fault   null write inside kernel code      -> not identified yet\n");
    kprintf("  panic   kernel panic                       -> not identified yet\n");
    return 1;
}
