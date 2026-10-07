#ifndef MUSAX_DRIVERS_PS2_KEYBOARD_H
#define MUSAX_DRIVERS_PS2_KEYBOARD_H

#include <stdint.h>

void ps2_keyboard_init(void);

/* Decodes one scan code from the controller and queues the key it produces. */
void ps2_keyboard_feed(uint8_t code);

/* Returns the next key code from musax/keys.h, or -1 when none is waiting. */
int ps2_keyboard_read(void);

#endif
