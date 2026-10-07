#include <stdint.h>
#include <musax/arch/x86_64/io.h>
#include <musax/arch/x86_64/pic.h>

#define PIC_MASTER_COMMAND 0x20
#define PIC_MASTER_DATA 0x21
#define PIC_SLAVE_COMMAND 0xA0
#define PIC_SLAVE_DATA 0xA1

#define PIC_ICW1_INIT 0x10
#define PIC_ICW1_ICW4 0x01
#define PIC_ICW4_8086 0x01
#define PIC_EOI 0x20
#define PIC_MASTER_SLAVE_LINE 0x04
#define PIC_SLAVE_CASCADE_IDENTITY 0x02
#define PIC_SLAVE_FIRST_IRQ 8
#define PIC_CASCADE_IRQ 2

static void io_delay(void)
{
    outb(0x80, 0);
}

void pic_init(void)
{
    outb(PIC_MASTER_COMMAND, PIC_ICW1_INIT | PIC_ICW1_ICW4);
    io_delay();
    outb(PIC_SLAVE_COMMAND, PIC_ICW1_INIT | PIC_ICW1_ICW4);
    io_delay();
    outb(PIC_MASTER_DATA, PIC_IRQ_BASE_VECTOR);
    io_delay();
    outb(PIC_SLAVE_DATA, PIC_IRQ_BASE_VECTOR + PIC_SLAVE_FIRST_IRQ);
    io_delay();
    outb(PIC_MASTER_DATA, PIC_MASTER_SLAVE_LINE);
    io_delay();
    outb(PIC_SLAVE_DATA, PIC_SLAVE_CASCADE_IDENTITY);
    io_delay();
    outb(PIC_MASTER_DATA, PIC_ICW4_8086);
    io_delay();
    outb(PIC_SLAVE_DATA, PIC_ICW4_8086);
    io_delay();

    outb(PIC_MASTER_DATA, 0xFF);
    outb(PIC_SLAVE_DATA, 0xFF);
}

static uint16_t data_port(unsigned irq)
{
    return irq < PIC_SLAVE_FIRST_IRQ ? PIC_MASTER_DATA : PIC_SLAVE_DATA;
}

void pic_mask(unsigned irq)
{
    uint16_t port = data_port(irq);

    outb(port, (uint8_t)(inb(port) | (1U << (irq % PIC_SLAVE_FIRST_IRQ))));
}

void pic_unmask(unsigned irq)
{
    uint16_t port = data_port(irq);

    outb(port, (uint8_t)(inb(port) & ~(1U << (irq % PIC_SLAVE_FIRST_IRQ))));
    if (irq >= PIC_SLAVE_FIRST_IRQ)
        pic_unmask(PIC_CASCADE_IRQ);
}

void pic_send_eoi(unsigned irq)
{
    if (irq >= PIC_SLAVE_FIRST_IRQ)
        outb(PIC_SLAVE_COMMAND, PIC_EOI);
    outb(PIC_MASTER_COMMAND, PIC_EOI);
}
