#include <stddef.h>
#include <musax/arch/x86_64/idt.h>
#include <musax/arch/x86_64/interrupts.h>
#include <musax/arch/x86_64/pic.h>

static irq_handler irq_handlers[PIC_IRQ_COUNT];

void irq_register(unsigned irq, irq_handler handler)
{
    if (irq < PIC_IRQ_COUNT)
        irq_handlers[irq] = handler;
}

void interrupt_dispatch(struct interrupt_frame *frame)
{
    unsigned irq;

    if (frame->vector < IDT_EXCEPTION_VECTORS)
        exception_dispatch(frame);

    irq = (unsigned)(frame->vector - PIC_IRQ_BASE_VECTOR);
    if (irq < PIC_IRQ_COUNT && irq_handlers[irq] != NULL)
        irq_handlers[irq]();
    pic_send_eoi(irq);
}
