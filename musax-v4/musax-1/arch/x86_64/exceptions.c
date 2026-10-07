#include <stdint.h>
#include <stdio.h>
#include <musax/arch/cpu.h>
#include <musax/arch/x86_64/idt.h>
#include <musax/crash.h>

#define VECTOR_PAGE_FAULT 14

#define PAGE_FAULT_PRESENT 0x1
#define PAGE_FAULT_WRITE 0x2
#define PAGE_FAULT_INSTRUCTION 0x10

static const char *const exception_names[IDT_EXCEPTION_VECTORS] = {
    "Divide Error",
    "Debug",
    "Non-Maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack-Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved",
    "x87 Floating-Point Exception",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Virtualization Exception",
    "Control Protection Exception",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Hypervisor Injection Exception",
    "VMM Communication Exception",
    "Security Exception",
    "Reserved",
};

static uint64_t read_cr2(void)
{
    uint64_t value;

    __asm__ volatile("mov %%cr2, %0" : "=r"(value));
    return value;
}

void exception_dispatch(const struct interrupt_frame *frame)
{
    static char summary[96];
    static char detail[96];
    struct crash_report report;
    uint64_t driver_start = crash_driver_text_start();
    uint64_t driver_end = crash_driver_text_end();

    cpu_disable_interrupts();

    report.class = crash_classify_exception(frame->vector, frame->rip, driver_start, driver_end);
    snprintf(summary, sizeof(summary), "#%lu %s at rip=0x%lx in %s code", frame->vector,
             exception_names[frame->vector % IDT_EXCEPTION_VECTORS], frame->rip,
             (frame->rip >= driver_start && frame->rip < driver_end) ? "driver" : "kernel");

    if (frame->vector == VECTOR_PAGE_FAULT) {
        snprintf(detail, sizeof(detail), "address 0x%lx: %s while %s", read_cr2(),
                 (frame->error_code & PAGE_FAULT_PRESENT) ? "protection violation" : "page not mapped",
                 (frame->error_code & PAGE_FAULT_INSTRUCTION) ? "fetching code"
                 : (frame->error_code & PAGE_FAULT_WRITE)     ? "writing"
                                                              : "reading");
    } else {
        snprintf(detail, sizeof(detail), "error=0x%lx rsp=0x%lx rflags=0x%lx", frame->error_code, frame->rsp,
                 frame->rflags);
    }

    report.summary = summary;
    report.detail = detail;
    crash_show(&report);
}
