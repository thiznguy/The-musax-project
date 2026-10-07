/* Stage 2, real-mode part: CPU check, A20 gate, BIOS memory map, kernel read, switch to protected mode. */

.intel_syntax noprefix
.include "layout.inc"

.code16

.section .text.entry,"ax"
.global stage2_entry

stage2_entry:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, REAL_MODE_STACK_TOP
    sti
    cld
    mov [boot_drive], dl
.ifdef DISPLAY_TALL
    mov ax, 0x1202
    mov bl, 0x30
    int 0x10
    mov ax, 0x0003
    int 0x10
    mov ax, 0x1112
    xor bl, bl
    int 0x10
.endif
    mov si, offset msg_banner
    call print16
    call check_long_mode
    call enable_a20
    call read_memory_map
    call capture_font
    call load_kernel
    jmp enter_protected_mode

.section .text

print16:
    lodsb
    test al, al
    jz .Lprint_done
    mov ah, 0x0E
    mov bx, 0x0007
    int 0x10
    jmp print16
.Lprint_done:
    ret

fail16:
    call print16
    cli
.Lfail_halt:
    hlt
    jmp .Lfail_halt

check_long_mode:
    pushfd
    pop eax
    mov ecx, eax
    xor eax, 0x200000
    push eax
    popfd
    pushfd
    pop eax
    push ecx
    popfd
    cmp eax, ecx
    je .Lno_long_mode
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb .Lno_long_mode
    mov eax, 0x80000001
    cpuid
    test edx, 0x20000000
    jz .Lno_long_mode
    ret
.Lno_long_mode:
    mov si, offset msg_no_long_mode
    jmp fail16

/* Returns AX=1 when address line 20 is enabled and AX=0 when memory wraps at 1 MiB. */
a20_is_enabled:
    push ds
    push es
    push si
    push di
    xor ax, ax
    mov ds, ax
    not ax
    mov es, ax
    mov si, 0x0500
    mov di, 0x0510
    mov al, [si]
    push ax
    mov al, es:[di]
    push ax
    mov byte ptr [si], 0x00
    mov byte ptr es:[di], 0xFF
    cmp byte ptr [si], 0xFF
    pop ax
    mov es:[di], al
    pop ax
    mov [si], al
    mov ax, 1
    jne .La20_state_known
    xor ax, ax
.La20_state_known:
    pop di
    pop si
    pop es
    pop ds
    ret

enable_a20:
    call a20_is_enabled
    test ax, ax
    jnz .La20_done
    mov ax, 0x2401
    int 0x15
    call a20_is_enabled
    test ax, ax
    jnz .La20_done
    in al, 0x92
    test al, 0x02
    jnz .La20_fast_set
    or al, 0x02
    and al, 0xFE
    out 0x92, al
.La20_fast_set:
    mov cx, 0xFFFF
.La20_settle:
    loop .La20_settle
    call a20_is_enabled
    test ax, ax
    jnz .La20_done
    mov si, offset msg_no_a20
    jmp fail16
.La20_done:
    ret

/* Copies the 8x16 ROM font of the video BIOS to FONT_ADDR; the kernel finds it through the boot information block.
   The block stays zero when the BIOS does not provide the font. */
capture_font:
    push ds
    push es
    push bp
    mov ax, 0x1130
    mov bh, 0x06
    int 0x10
    mov ax, es
    mov bx, bp
    pop bp
    mov dx, es
    pop es
    push es
    test dx, dx
    jz .Lfont_done
    mov ds, ax
    mov si, bx
    xor ax, ax
    mov es, ax
    mov di, FONT_ADDR
    mov cx, FONT_BYTES / 2
    cld
    rep movsw
    xor ax, ax
    mov ds, ax
    mov dword ptr [BOOTINFO_ADDR + BOOTINFO_FONT_OFFSET], FONT_ADDR
.Lfont_done:
    pop es
    pop ds
    ret

read_memory_map:
    mov dword ptr [BOOTINFO_ADDR + BOOTINFO_MAGIC_OFFSET], BOOTINFO_MAGIC
    movzx eax, byte ptr [boot_drive]
    mov [BOOTINFO_ADDR + BOOTINFO_DRIVE_OFFSET], eax
    mov dword ptr [BOOTINFO_ADDR + BOOTINFO_FONT_OFFSET], 0
    mov di, BOOTINFO_ADDR + BOOTINFO_ENTRIES_OFFSET
    xor ebx, ebx
.Lmemmap_next:
    mov dword ptr es:[di + 20], 1
    mov eax, 0xE820
    mov edx, 0x534D4150
    mov ecx, BOOTINFO_ENTRY_SIZE
    int 0x15
    jc .Lmemmap_finish
    cmp eax, 0x534D4150
    jne .Lmemmap_fail
    mov eax, es:[di + 8]
    or eax, es:[di + 12]
    jz .Lmemmap_skip
    add di, BOOTINFO_ENTRY_SIZE
    inc word ptr [memmap_count]
    cmp word ptr [memmap_count], BOOTINFO_MAX_ENTRIES
    jae .Lmemmap_finish
.Lmemmap_skip:
    test ebx, ebx
    jnz .Lmemmap_next
.Lmemmap_finish:
    movzx eax, word ptr [memmap_count]
    test eax, eax
    jz .Lmemmap_fail
    mov [BOOTINFO_ADDR + BOOTINFO_COUNT_OFFSET], eax
    ret
.Lmemmap_fail:
    mov si, offset msg_no_memmap
    jmp fail16

load_kernel:
    mov word ptr [dap_segment], KERNEL_BUFFER_SEGMENT
    mov word ptr [dap_offset], 0
    mov dword ptr [dap_lba], KERNEL_LBA
    mov word ptr [sectors_remaining], KERNEL_SECTORS
.Lkernel_chunk:
    mov ax, [sectors_remaining]
    test ax, ax
    jz .Lkernel_loaded
    cmp ax, KERNEL_READ_CHUNK_SECTORS
    jbe .Lkernel_count_ready
    mov ax, KERNEL_READ_CHUNK_SECTORS
.Lkernel_count_ready:
    mov [dap_count], ax
    sub [sectors_remaining], ax
    mov dl, [boot_drive]
    mov si, offset dap
    mov ah, 0x42
    int 0x13
    jc .Lkernel_read_failed
    add word ptr [dap_segment], KERNEL_READ_CHUNK_SEGMENT_STEP
    add dword ptr [dap_lba], KERNEL_READ_CHUNK_SECTORS
    jmp .Lkernel_chunk
.Lkernel_loaded:
    ret
.Lkernel_read_failed:
    mov si, offset msg_kernel_read
    jmp fail16

enter_protected_mode:
    cli
    lgdt [gdt_descriptor]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    .byte 0x66, 0xEA
    .long protected_entry
    .word GDT_CODE32

.section .data

/* BIOS drive number the boot sector was loaded from. */
boot_drive:
    .byte 0

/* Number of BIOS E820 entries stored in the boot information block. */
memmap_count:
    .word 0

/* Kernel sectors that remain to be read. */
sectors_remaining:
    .word 0

/* Disk address packet for INT 13h AH=42h kernel reads. */
dap:
    .byte 0x10
    .byte 0
dap_count:
    .word 0
dap_offset:
    .word 0
dap_segment:
    .word 0
dap_lba:
    .quad 0

/* Global descriptor table used while leaving real mode: null, code32, data32, code64. */
.align 8
gdt:
    .quad 0x0000000000000000
    .quad 0x00CF9A000000FFFF
    .quad 0x00CF92000000FFFF
    .quad 0x00AF9A000000FFFF
gdt_end:

gdt_descriptor:
    .word gdt_end - gdt - 1
    .long gdt

msg_banner:
    .asciz "musax-1 stage2\r\n"
msg_no_long_mode:
    .asciz "musax-1 stage2: CPU lacks 64-bit mode"
msg_no_a20:
    .asciz "musax-1 stage2: A20 gate would not enable"
msg_no_memmap:
    .asciz "musax-1 stage2: BIOS E820 memory map unavailable"
msg_kernel_read:
    .asciz "musax-1 stage2: kernel read failed"
