#include <stdint.h>
#include <string.h>
#include <musax/arch/cpuinfo.h>

#define CPUID_VENDOR_LEAF 0x0
#define CPUID_SIGNATURE_LEAF 0x1
#define CPUID_EXTENDED_MAX_LEAF 0x80000000U
#define CPUID_BRAND_FIRST_LEAF 0x80000002U
#define CPUID_BRAND_LAST_LEAF 0x80000004U

static void cpuid(uint32_t leaf, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx)
{
    __asm__ volatile("cpuid" : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx) : "a"(leaf), "c"(0));
}

static void store_word(char *out, uint32_t word)
{
    for (unsigned i = 0; i < 4; i++)
        out[i] = (char)((word >> (8 * i)) & 0xFF);
}

void arch_cpu_describe(struct cpu_info *info)
{
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t base_family;
    uint32_t base_model;

    memset(info, 0, sizeof(*info));

    cpuid(CPUID_VENDOR_LEAF, &eax, &ebx, &ecx, &edx);
    store_word(info->vendor, ebx);
    store_word(info->vendor + 4, edx);
    store_word(info->vendor + 8, ecx);

    cpuid(CPUID_SIGNATURE_LEAF, &eax, &ebx, &ecx, &edx);
    base_family = (eax >> 8) & 0xF;
    base_model = (eax >> 4) & 0xF;
    info->family = base_family == 0xF ? base_family + ((eax >> 20) & 0xFF) : base_family;
    info->model = (base_family == 0x6 || base_family == 0xF) ? base_model + (((eax >> 16) & 0xF) << 4) : base_model;
    info->stepping = eax & 0xF;

    cpuid(CPUID_EXTENDED_MAX_LEAF, &eax, &ebx, &ecx, &edx);
    if (eax >= CPUID_BRAND_LAST_LEAF) {
        char raw[49] = { 0 };
        const char *start = raw;

        for (uint32_t leaf = CPUID_BRAND_FIRST_LEAF; leaf <= CPUID_BRAND_LAST_LEAF; leaf++) {
            char *slot = raw + (leaf - CPUID_BRAND_FIRST_LEAF) * 16;

            cpuid(leaf, &eax, &ebx, &ecx, &edx);
            store_word(slot, eax);
            store_word(slot + 4, ebx);
            store_word(slot + 8, ecx);
            store_word(slot + 12, edx);
        }
        while (*start == ' ')
            start++;
        strlcpy(info->brand, start, sizeof(info->brand));
    }
}
