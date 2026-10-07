/* Stage 1: 512-byte MBR boot sector that loads stage 2 with BIOS extended disk reads. */

.intel_syntax noprefix
.include "layout.inc"

.code16
.section .text
.global stage1_entry

stage1_entry:
    .byte 0xEA
    .word .Lnormalized
    .word 0x0000
.Lnormalized:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, STAGE1_LOAD_ADDR
    sti
    cld
    mov [boot_drive], dl
    mov ah, 0x41
    mov bx, 0x55AA
    int 0x13
    jc .Lno_extensions
    cmp bx, 0xAA55
    jne .Lno_extensions
    mov dl, [boot_drive]
    mov si, offset disk_address_packet
    mov ah, 0x42
    int 0x13
    jc .Lread_failed
    mov dl, [boot_drive]
    mov ax, STAGE2_LOAD_ADDR
    jmp ax

.Lno_extensions:
    mov si, offset msg_no_extensions
    jmp fail

.Lread_failed:
    mov si, offset msg_read_failed

fail:
    lodsb
    test al, al
    jz .Lhalt
    mov ah, 0x0E
    mov bx, 0x0007
    int 0x10
    jmp fail
.Lhalt:
    cli
    hlt
    jmp .Lhalt

/* BIOS drive number received in DL at power-on. */
boot_drive:
    .byte 0

/* Disk address packet for the INT 13h AH=42h extended read of stage 2. */
disk_address_packet:
    .byte 0x10
    .byte 0
    .word STAGE2_SECTORS
    .word STAGE2_LOAD_ADDR
    .word 0x0000
    .quad STAGE2_LBA

msg_no_extensions:
    .asciz "musax-1 stage1: BIOS lacks extended disk reads"
msg_read_failed:
    .asciz "musax-1 stage1: stage2 read failed"

/* Empty partition table region followed by the boot signature. */
.org 446
.fill 64, 1, 0
.word 0xAA55
