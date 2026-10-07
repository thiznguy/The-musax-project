#ifndef MUSAX_ARCH_X86_64_GDT_H
#define MUSAX_ARCH_X86_64_GDT_H

#define GDT_KERNEL_CODE 0x08
#define GDT_KERNEL_DATA 0x10
#define GDT_TSS 0x18

/* Interrupt stack table slots (1-based) with their own stacks, so these faults never run on a broken stack. */
#define IST_DOUBLE_FAULT 1
#define IST_NMI 2
#define IST_MACHINE_CHECK 3

void gdt_init(void);

#endif
