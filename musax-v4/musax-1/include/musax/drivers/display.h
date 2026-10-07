#ifndef MUSAX_DRIVERS_DISPLAY_H
#define MUSAX_DRIVERS_DISPLAY_H

#include <stdbool.h>
#include <stdint.h>

struct display_mode {
    uint32_t *framebuffer;
    int width;
    int height;
    int pitch_pixels;
};

/* Switches the Bochs/QEMU display adapter from text mode to a 32-bit linear framebuffer; returns false when no such adapter exists. */
bool display_enter_graphics(int width, int height, struct display_mode *mode);

/* Returns the adapter to VGA text mode. */
void display_leave_graphics(void);

/* True while the adapter shows the framebuffer. */
bool display_is_graphical(void);

#endif
