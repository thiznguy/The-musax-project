#include <stdint.h>
#include <musax/arch/x86_64/gdt.h>
#include <musax/arch/x86_64/tss.h>

#define IST_STACK_BYTES 16384
#define IST_STACKS_USED 3

struct gdt_pointer {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

/* Null descriptor, 64-bit kernel code segment, kernel data segment, then the two-slot TSS descriptor. */
static uint64_t gdt[5] __attribute__((aligned(8))) = {
    0x0000000000000000ULL,
    0x00AF9B000000FFFFULL,
    0x00CF93000000FFFFULL,
    0,
    0,
};

static struct tss tss;
static uint8_t ist_stacks[IST_STACKS_USED][IST_STACK_BYTES] __attribute__((aligned(16)));

void gdt_init(void)
{
    const struct gdt_pointer pointer = {
        .limit = sizeof(gdt) - 1,
        .base = (uint64_t)(uintptr_t)gdt,
    };

    for (unsigned i = 0; i < IST_STACKS_USED; i++)
        tss.ist[i] = (uint64_t)(uintptr_t)&ist_stacks[i][IST_STACK_BYTES];
    tss.iomap_base = sizeof(tss);
    tss_encode_descriptor(&gdt[GDT_TSS / 8], (uint64_t)(uintptr_t)&tss, sizeof(tss) - 1);

    __asm__ volatile(
        "lgdt %0\n\t"
        "pushq $%c1\n\t"
        "leaq 1f(%%rip), %%rax\n\t"
        "pushq %%rax\n\t"
        "lretq\n"
        "1:\n\t"
        "movw $%c2, %%ax\n\t"
        "movw %%ax, %%ds\n\t"
        "movw %%ax, %%es\n\t"
        "movw %%ax, %%fs\n\t"
        "movw %%ax, %%gs\n\t"
        "movw %%ax, %%ss\n\t"
        "movw $%c3, %%ax\n\t"
        "ltr %%ax"
        :
        : "m"(pointer), "i"(GDT_KERNEL_CODE), "i"(GDT_KERNEL_DATA), "i"(GDT_TSS)
        : "rax", "memory");
}
