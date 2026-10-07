/* Interrupt entry stubs for exception vectors 0-31 and hardware interrupt vectors 32-47, plus the shared register-saving path. */

.intel_syntax noprefix

.macro isr_stub vector, has_error
isr_stub_\vector:
    .if \has_error == 0
        push 0
    .endif
    push \vector
    jmp isr_common
.endm

.section .text

isr_stub 0, 0
isr_stub 1, 0
isr_stub 2, 0
isr_stub 3, 0
isr_stub 4, 0
isr_stub 5, 0
isr_stub 6, 0
isr_stub 7, 0
isr_stub 9, 0
isr_stub 15, 0
isr_stub 16, 0
isr_stub 18, 0
isr_stub 19, 0
isr_stub 20, 0
isr_stub 22, 0
isr_stub 23, 0
isr_stub 24, 0
isr_stub 25, 0
isr_stub 26, 0
isr_stub 27, 0
isr_stub 28, 0
isr_stub 31, 0
isr_stub 32, 0
isr_stub 33, 0
isr_stub 34, 0
isr_stub 35, 0
isr_stub 36, 0
isr_stub 37, 0
isr_stub 38, 0
isr_stub 39, 0
isr_stub 40, 0
isr_stub 41, 0
isr_stub 42, 0
isr_stub 43, 0
isr_stub 44, 0
isr_stub 45, 0
isr_stub 46, 0
isr_stub 47, 0
isr_stub 8, 1
isr_stub 10, 1
isr_stub 11, 1
isr_stub 12, 1
isr_stub 13, 1
isr_stub 14, 1
isr_stub 17, 1
isr_stub 21, 1
isr_stub 29, 1
isr_stub 30, 1

isr_common:
    push rax
    push rcx
    push rdx
    push rbx
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    mov rdi, rsp
    cld
    call interrupt_dispatch
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbp
    pop rbx
    pop rdx
    pop rcx
    pop rax
    add rsp, 16
    iretq

/* Addresses of the interrupt stubs, indexed by vector number. */
.section .rodata
.align 8
.global isr_stub_table
isr_stub_table:
.altmacro
.macro isr_table_entry n
    .quad isr_stub_\n
.endm
.set vector_index, 0
.rept 48
    isr_table_entry %vector_index
    .set vector_index, vector_index + 1
.endr
