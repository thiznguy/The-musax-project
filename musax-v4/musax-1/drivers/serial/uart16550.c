#include <stdbool.h>
#include <stdint.h>
#include <musax/arch/x86_64/io.h>
#include <musax/drivers/uart16550.h>

#define COM1_BASE 0x3F8

#define UART_DATA 0
#define UART_INTERRUPT_ENABLE 1
#define UART_FIFO_CONTROL 2
#define UART_LINE_CONTROL 3
#define UART_MODEM_CONTROL 4
#define UART_LINE_STATUS 5

#define UART_LCR_8N1 0x03
#define UART_LCR_DLAB 0x80
#define UART_DIVISOR_115200 1
#define UART_FCR_ENABLE_CLEAR_14_BYTE 0xC7
#define UART_MCR_DTR_RTS_OUT2 0x0B
#define UART_MCR_LOOPBACK 0x1E
#define UART_LSR_DATA_READY 0x01
#define UART_LSR_TRANSMIT_EMPTY 0x20
#define UART_LOOPBACK_PATTERN 0xAE

static bool uart_present;

void uart16550_init(void)
{
    outb(COM1_BASE + UART_INTERRUPT_ENABLE, 0x00);
    outb(COM1_BASE + UART_LINE_CONTROL, UART_LCR_DLAB);
    outb(COM1_BASE + UART_DATA, UART_DIVISOR_115200 & 0xFF);
    outb(COM1_BASE + UART_INTERRUPT_ENABLE, UART_DIVISOR_115200 >> 8);
    outb(COM1_BASE + UART_LINE_CONTROL, UART_LCR_8N1);
    outb(COM1_BASE + UART_FIFO_CONTROL, UART_FCR_ENABLE_CLEAR_14_BYTE);

    outb(COM1_BASE + UART_MODEM_CONTROL, UART_MCR_LOOPBACK);
    outb(COM1_BASE + UART_DATA, UART_LOOPBACK_PATTERN);
    uart_present = inb(COM1_BASE + UART_DATA) == UART_LOOPBACK_PATTERN;

    outb(COM1_BASE + UART_MODEM_CONTROL, UART_MCR_DTR_RTS_OUT2);
}

void uart16550_write(char c)
{
    if (!uart_present)
        return;
    while (!(inb(COM1_BASE + UART_LINE_STATUS) & UART_LSR_TRANSMIT_EMPTY))
        ;
    outb(COM1_BASE + UART_DATA, (uint8_t)c);
}

int uart16550_read(void)
{
    if (!uart_present)
        return -1;
    if (!(inb(COM1_BASE + UART_LINE_STATUS) & UART_LSR_DATA_READY))
        return -1;
    return inb(COM1_BASE + UART_DATA);
}
