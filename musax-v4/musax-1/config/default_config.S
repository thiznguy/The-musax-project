/* Default configuration text embedded into the kernel image and NUL-terminated. */

.section .rodata
.global default_config_start
.global default_config_end

default_config_start:
    .incbin "musax.conf"
default_config_end:
    .byte 0
