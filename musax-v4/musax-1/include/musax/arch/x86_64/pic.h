#ifndef MUSAX_ARCH_X86_64_PIC_H
#define MUSAX_ARCH_X86_64_PIC_H

#include <stdint.h>

#define PIC_IRQ_BASE_VECTOR 32
#define PIC_IRQ_COUNT 16

/* Remaps the two 8259 controllers to vectors 32-47 and masks every line. */
void pic_init(void);
void pic_mask(unsigned irq);
void pic_unmask(unsigned irq);
void pic_send_eoi(unsigned irq);

#endif
