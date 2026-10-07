#include <stdint.h>
#include <musax/arch/x86_64/tss.h>

#define TSS_ACCESS_AVAILABLE_64BIT 0x89ULL

void tss_encode_descriptor(uint64_t out[2], uint64_t base, uint32_t limit)
{
    out[0] = (uint64_t)(limit & 0xFFFF) | ((base & 0xFFFFFFULL) << 16) | (TSS_ACCESS_AVAILABLE_64BIT << 40) |
             ((uint64_t)((limit >> 16) & 0xF) << 48) | (((base >> 24) & 0xFFULL) << 56);
    out[1] = base >> 32;
}
