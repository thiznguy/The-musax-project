#include <stdint.h>
#include <musax/arch/arch.h>
#include <musax/arch/cpu.h>
#include <musax/arch/x86_64/gdt.h>
#include <musax/arch/x86_64/idt.h>
#include <musax/arch/x86_64/io.h>
#include <musax/arch/x86_64/pic.h>

#define KBC_COMMAND_PORT 0x64
#define KBC_STATUS_INPUT_FULL 0x02
#define KBC_COMMAND_PULSE_RESET 0xFE
#define KBC_WAIT_SPINS 0x100000U
#define RESET_SETTLE_SPINS 0x1000000U

struct empty_idt_pointer {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

void arch_init(void)
{
    gdt_init();
    idt_init();
}

void arch_init_interrupts(void)
{
    pic_init();
}

void arch_reboot(void)
{
    const struct empty_idt_pointer empty = { .limit = 0, .base = 0 };
    uint32_t spins = KBC_WAIT_SPINS;

    cpu_disable_interrupts();

    while ((inb(KBC_COMMAND_PORT) & KBC_STATUS_INPUT_FULL) && spins-- > 0)
        cpu_relax();
    outb(KBC_COMMAND_PORT, KBC_COMMAND_PULSE_RESET);

    for (volatile uint32_t settle = 0; settle < RESET_SETTLE_SPINS; settle++)
        cpu_relax();

    __asm__ volatile("lidt %0\n\tint3" : : "m"(empty));
    cpu_halt_forever();
}
