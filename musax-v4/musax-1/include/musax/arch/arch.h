#ifndef MUSAX_ARCH_ARCH_H
#define MUSAX_ARCH_ARCH_H

/* Architecture entry points implemented under kernel/arch/<arch>/. */
/* Installs the CPU tables (segments, task state, exception handlers). */
void arch_init(void);

/* Remaps the interrupt controller; every hardware interrupt line stays masked until it is unmasked. */
void arch_init_interrupts(void);
__attribute__((noreturn)) void arch_reboot(void);

#endif
