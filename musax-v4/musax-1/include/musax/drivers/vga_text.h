#ifndef MUSAX_DRIVERS_VGA_TEXT_H
#define MUSAX_DRIVERS_VGA_TEXT_H

#include <stddef.h>
#include <stdint.h>

#define VGA_COLUMNS 80
#define VGA_MAX_ROWS 50

/* Reads the text mode the firmware set up; call once before any other function of the driver. */
void vga_text_init(void);
size_t vga_text_rows(void);

void vga_text_clear(void);
void vga_text_putc(char c);
void vga_text_put_cell(size_t row, size_t column, uint8_t c, uint8_t attribute);
void vga_text_set_cursor(size_t row, size_t column);
void vga_text_hide_cursor(void);

#endif
