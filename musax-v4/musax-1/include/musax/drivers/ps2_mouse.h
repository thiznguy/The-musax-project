#ifndef MUSAX_DRIVERS_PS2_MOUSE_H
#define MUSAX_DRIVERS_PS2_MOUSE_H

#include <stdbool.h>
#include <stdint.h>

#define MOUSE_BUTTON_LEFT 0x01
#define MOUSE_BUTTON_RIGHT 0x02
#define MOUSE_BUTTON_MIDDLE 0x04

/* Movement since the previous event; dy is positive when the mouse moves toward the user. */
struct mouse_event {
    int dx;
    int dy;
    uint8_t buttons;
};

/* Enables the mouse port and data reporting; returns false when no mouse answers. */
bool ps2_mouse_init(void);

/* Decodes one byte of the three-byte mouse packet. */
void ps2_mouse_feed(uint8_t data);

bool ps2_mouse_read(struct mouse_event *event);

#endif
