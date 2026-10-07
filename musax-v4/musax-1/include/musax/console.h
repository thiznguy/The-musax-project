#ifndef MUSAX_CONSOLE_H
#define MUSAX_CONSOLE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CONSOLE_COLUMNS 80
#define CONSOLE_MAX_ROWS 50

/* The sixteen VGA text colors. */
#define CONSOLE_COLOR_BLACK 0
#define CONSOLE_COLOR_BLUE 1
#define CONSOLE_COLOR_GREEN 2
#define CONSOLE_COLOR_CYAN 3
#define CONSOLE_COLOR_RED 4
#define CONSOLE_COLOR_MAGENTA 5
#define CONSOLE_COLOR_BROWN 6
#define CONSOLE_COLOR_LIGHT_GREY 7
#define CONSOLE_COLOR_DARK_GREY 8
#define CONSOLE_COLOR_LIGHT_BLUE 9
#define CONSOLE_COLOR_LIGHT_GREEN 10
#define CONSOLE_COLOR_LIGHT_CYAN 11
#define CONSOLE_COLOR_LIGHT_RED 12
#define CONSOLE_COLOR_LIGHT_MAGENTA 13
#define CONSOLE_COLOR_YELLOW 14
#define CONSOLE_COLOR_WHITE 15

/* A cell attribute holds the foreground color in the low nibble and a background color from 0 to 7 in the high nibble. */
#define CONSOLE_ATTRIBUTE(foreground, background) ((uint8_t)(((background) << 4) | (foreground)))
#define CONSOLE_ATTRIBUTE_NORMAL CONSOLE_ATTRIBUTE(CONSOLE_COLOR_LIGHT_GREY, CONSOLE_COLOR_BLACK)
#define CONSOLE_ATTRIBUTE_INVERSE CONSOLE_ATTRIBUTE(CONSOLE_COLOR_BLACK, CONSOLE_COLOR_LIGHT_GREY)

void console_init(void);
void console_putc(char c);
void console_puts(const char *string);
void console_clear(void);

/* Number of text rows of the screen, from 25 to CONSOLE_MAX_ROWS. */
size_t console_rows(void);

/* Diverts console output to a handler until console_pop_sink; sinks nest. clear may be NULL to ignore screen clears. */
void console_push_sink(void (*put)(char c, void *context), void (*clear)(void *context), void *context);
void console_pop_sink(void);

/* A graphical desktop owns the screen: the text-mode full-screen functions below do nothing while this is set. */
void console_set_graphical(bool graphical);
bool console_is_graphical(void);

/* Registers a function that console_read_key and the desktop loop run while they wait for input. */
void console_add_idle_hook(void (*hook)(void));
void console_run_idle_hooks(void);

/* The crash screen takes the display over: drops all sinks and runs the hook that returns a graphical display to text mode. */
void console_set_crash_hook(void (*hook)(void));
void console_prepare_for_crash(void);

/* Blocks until a key arrives; console_poll_key returns -1 instead when no key is waiting. Values come from musax/keys.h. */
int console_read_key(void);
int console_poll_key(void);

/* Full-screen output for text user interfaces. Characters above 0x7F are IBM code page 437 glyphs (blocks, box lines). */
void console_tui_begin(void);
void console_tui_end(void);
void console_put_cell(size_t row, size_t column, uint8_t c, uint8_t attribute);
void console_set_cursor(size_t row, size_t column);
void console_hide_cursor(void);

#endif
