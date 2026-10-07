#include <stddef.h>
#include <stdint.h>
#include <musax/arch/x86_64/gdt.h>
#include <musax/arch/x86_64/idt.h>

#define IDT_ENTRIES 256
#define VECTOR_NMI 2
#define VECTOR_DOUBLE_FAULT 8
#define VECTOR_MACHINE_CHECK 18
#define IDT_INTERRUPT_GATE_PRESENT 0x8E

struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t type_attributes;
    uint16_t offset_middle;
    uint32_t offset_high;
    uint32_t reserved;
} __attribute__((packed));

struct idt_pointer {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

extern const uint64_t isr_stub_table[IDT_HANDLED_VECTORS];

static struct idt_entry idt[IDT_ENTRIES] __attribute__((aligned(16)));

static uint8_t ist_for_vector(size_t vector)
{
    switch (vector) {
    case VECTOR_NMI:
        return IST_NMI;
    case VECTOR_DOUBLE_FAULT:
        return IST_DOUBLE_FAULT;
    case VECTOR_MACHINE_CHECK:
        return IST_MACHINE_CHECK;
    default:
        return 0;
    }
}

static void idt_set_gate(size_t vector, uint64_t handler)
{
    idt[vector].offset_low = (uint16_t)(handler & 0xFFFF);
    idt[vector].selector = GDT_KERNEL_CODE;
    idt[vector].ist = ist_for_vector(vector);
    idt[vector].type_attributes = IDT_INTERRUPT_GATE_PRESENT;
    idt[vector].offset_middle = (uint16_t)((handler >> 16) & 0xFFFF);
    idt[vector].offset_high = (uint32_t)(handler >> 32);
    idt[vector].reserved = 0;
}

void idt_init(void)
{
    const struct idt_pointer pointer = {
        .limit = sizeof(idt) - 1,
        .base = (uint64_t)(uintptr_t)idt,
    };

    for (size_t vector = 0; vector < IDT_HANDLED_VECTORS; vector++)
        idt_set_gate(vector, isr_stub_table[vector]);

    __asm__ volatile("lidt %0" : : "m"(pointer));
}
