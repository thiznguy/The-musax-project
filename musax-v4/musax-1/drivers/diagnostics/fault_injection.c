#include <stdint.h>
#include <musax/drivers/fault_injection.h>

void fault_inject_driver_null_write(void)
{
    volatile uintptr_t address = 0;

    *(volatile uint32_t *)address = 0xDEAD;
}
