#ifndef MUSAX_DRIVERS_UART16550_H
#define MUSAX_DRIVERS_UART16550_H

void uart16550_init(void);
void uart16550_write(char c);
int uart16550_read(void);

#endif
