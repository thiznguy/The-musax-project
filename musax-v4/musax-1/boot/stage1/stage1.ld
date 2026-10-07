ENTRY(stage1_entry)

SECTIONS
{
    . = 0x7C00;
    .text : { *(.text .text.*) }
    /DISCARD/ : { *(.note*) *(.comment) *(.eh_frame*) }
}
