#ifndef MUSAX_DRIVERS_I8042_H
#define MUSAX_DRIVERS_I8042_H

#include <stdbool.h>
#include <stdint.h>

/* Moves every byte waiting in the controller to the keyboard or mouse decoder it belongs to. */
void i8042_poll(void);

bool i8042_write_command(uint8_t command);
bool i8042_write_data(uint8_t data);

/* Waits briefly for one byte; from_aux tells whether the mouse port produced it. */
bool i8042_read_data(uint8_t *data, bool *from_aux);

#endif
