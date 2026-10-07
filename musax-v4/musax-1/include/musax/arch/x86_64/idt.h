#ifndef MUSAX_ARCH_X86_64_IDT_H
#define MUSAX_ARCH_X86_64_IDT_H

#include <stdint.h>

#define IDT_EXCEPTION_VECTORS 32
#define IDT_HANDLED_VECTORS 48

/* Register state saved by the interrupt stubs, lowest address first. */
struct interrupt_frame {
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rbp;
    uint64_t rbx;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rax;
    uint64_t vector;
    uint64_t error_code;
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
};

void idt_init(void);
__attribute__((noreturn)) void exception_dispatch(const struct interrupt_frame *frame);

#endif
