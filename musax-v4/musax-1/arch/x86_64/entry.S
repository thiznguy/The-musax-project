/* Kernel entry point reached from stage 2 in 64-bit mode with RDI holding the boot information block. */

.intel_syntax noprefix

.section .text.entry,"ax"
.global _start

_start:
    cli
    cld
    mov r12, rdi
    lea rsp, [rip + __stack_top]
    lea rdi, [rip + __bss_start]
    lea rcx, [rip + __bss_end]
    sub rcx, rdi
    xor eax, eax
    rep stosb
    mov rdi, r12
    xor ebp, ebp
    call kmain
.Lhalt:
    cli
    hlt
    jmp .Lhalt
