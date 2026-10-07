#ifndef MUSAX_ARCH_X86_64_INTERRUPTS_H
#define MUSAX_ARCH_X86_64_INTERRUPTS_H

#include <musax/arch/x86_64/idt.h>

typedef void (*irq_handler)(void);

/* Installs the handler for a hardware interrupt line (0-15); the line stays masked until pic_unmask. */
void irq_register(unsigned irq, irq_handler handler);

/* Entered from the interrupt stubs for every vector. */
void interrupt_dispatch(struct interrupt_frame *frame);

#endif
