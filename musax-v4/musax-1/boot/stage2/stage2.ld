ENTRY(stage2_entry)

SECTIONS
{
    . = 0x7E00;
    .text : { KEEP(*(.text.entry)) *(.text .text.*) }
    .rodata : { *(.rodata .rodata.*) }
    .data : { *(.data .data.*) }
    __stage2_end = .;
    ASSERT(__stage2_end <= 0x9E00, "stage 2 does not fit in its 16 reserved sectors")
    /DISCARD/ : { *(.note*) *(.comment) *(.eh_frame*) }
}
