#ifndef MUSAX_SCREEN_H
#define MUSAX_SCREEN_H

#include <stddef.h>
#include <stdint.h>
#include <musax/console.h>

/* Off-screen frame for text user interfaces: draw into it, then flush to send only the cells that changed. */
void screen_begin(void);
void screen_end(void);
void screen_clear(uint8_t attribute);
void screen_put(size_t row, size_t column, uint8_t c, uint8_t attribute);
void screen_fill_row(size_t row, uint8_t c, uint8_t attribute);
void screen_write(size_t row, size_t column, const char *text, size_t length, uint8_t attribute);
void screen_flush(size_t cursor_row, size_t cursor_column);

/* Same as screen_flush but leaves the cursor hidden. */
void screen_flush_hidden(void);

#endif
