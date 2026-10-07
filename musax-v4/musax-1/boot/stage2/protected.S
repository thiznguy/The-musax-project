/* Stage 2, protected-mode part: kernel copy above 1 MiB, identity paging, long mode activation. */

.intel_syntax noprefix
.include "layout.inc"

.code32
.section .text
.global protected_entry

protected_entry:
    mov ax, GDT_DATA32
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, REAL_MODE_STACK_TOP
    cld

    mov esi, KERNEL_BUFFER_ADDR
    mov edi, KERNEL_LOAD_ADDR
    mov ecx, KERNEL_SECTORS * 128
    rep movsd

    mov edi, PAGE_TABLE_PML4
    xor eax, eax
    mov ecx, PAGE_TABLE_BYTES / 4
    rep stosd

    mov dword ptr [PAGE_TABLE_PML4], PAGE_TABLE_PDPT + 0x3

    mov edi, PAGE_TABLE_PDPT
    mov eax, PAGE_TABLE_PD + 0x3
    mov ecx, 4
.Lfill_pdpt:
    mov [edi], eax
    add eax, 0x1000
    add edi, 8
    loop .Lfill_pdpt

    mov edi, PAGE_TABLE_PD
    mov eax, 0x83
    mov ecx, 2048
.Lfill_pd:
    mov [edi], eax
    add eax, 0x200000
    add edi, 8
    loop .Lfill_pd

    mov eax, cr4
    or eax, 0x20
    mov cr4, eax

    mov eax, PAGE_TABLE_PML4
    mov cr3, eax

    mov ecx, 0xC0000080
    rdmsr
    or eax, 0x100
    wrmsr

    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax

    .byte 0xEA
    .long long_entry
    .word GDT_CODE64
