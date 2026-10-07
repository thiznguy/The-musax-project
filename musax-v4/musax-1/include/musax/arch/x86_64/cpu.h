#ifndef MUSAX_ARCH_X86_64_CPU_H
#define MUSAX_ARCH_X86_64_CPU_H

#include <stdbool.h>
#include <stdint.h>

#define CPU_FLAGS_INTERRUPT_ENABLE (1UL << 9)

static inline void cpu_disable_interrupts(void)
{
    __asm__ volatile("cli" : : : "memory");
}

static inline void cpu_enable_interrupts(void)
{
    __asm__ volatile("sti" : : : "memory");
}

static inline bool cpu_interrupts_enabled(void)
{
    uint64_t flags;

    __asm__ volatile("pushfq\n\tpopq %0" : "=r"(flags));
    return (flags & CPU_FLAGS_INTERRUPT_ENABLE) != 0;
}

/* Disables interrupts and returns the previous flags for cpu_restore_interrupts. */
static inline uint64_t cpu_save_interrupts(void)
{
    uint64_t flags;

    __asm__ volatile("pushfq\n\tpopq %0\n\tcli" : "=r"(flags) : : "memory");
    return flags;
}

static inline void cpu_restore_interrupts(uint64_t flags)
{
    if (flags & CPU_FLAGS_INTERRUPT_ENABLE)
        cpu_enable_interrupts();
}

static inline void cpu_relax(void)
{
    __asm__ volatile("pause" : : : "memory");
}

/* Sleeps until the next interrupt; interrupts must be enabled or the processor never wakes. */
static inline void cpu_idle(void)
{
    __asm__ volatile("hlt" : : : "memory");
}

static inline __attribute__((noreturn)) void cpu_halt_forever(void)
{
    cpu_disable_interrupts();
    for (;;)
        __asm__ volatile("hlt");
}

#endif
