/* Stage 2, long-mode part: hands control to the kernel entry point with RDI pointing at the boot information block. */

.intel_syntax noprefix
.include "layout.inc"

.code64
.section .text
.global long_entry

long_entry:
    xor eax, eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov edi, BOOTINFO_ADDR
    mov eax, KERNEL_LOAD_ADDR
    jmp rax
