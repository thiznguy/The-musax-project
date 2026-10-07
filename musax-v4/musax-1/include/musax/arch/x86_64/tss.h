#ifndef MUSAX_ARCH_X86_64_TSS_H
#define MUSAX_ARCH_X86_64_TSS_H

#include <stdint.h>

#define TSS_IST_COUNT 7

/* 64-bit task state segment; only the interrupt stack table is used. */
struct tss {
    uint32_t reserved0;
    uint64_t rsp[3];
    uint64_t reserved1;
    uint64_t ist[TSS_IST_COUNT];
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iomap_base;
} __attribute__((packed));

/* Builds the 16-byte GDT system descriptor of an available 64-bit TSS. */
void tss_encode_descriptor(uint64_t out[2], uint64_t base, uint32_t limit);

#endif
