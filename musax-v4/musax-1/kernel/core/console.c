#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <musax/arch/cpu.h>
#include <musax/console.h>
#include <musax/cp437.h>
#include <musax/drivers/ps2_keyboard.h>
#include <musax/drivers/timer.h>
#include <musax/drivers/uart16550.h>
#include <musax/drivers/vga_text.h>
#include <musax/keys.h>

#define ASCII_DELETE 0x7F
#define ESCAPE_FOLLOW_TIMEOUT_MS 25
#define ANSI_RESET_ATTRIBUTES "\033[0m"
#define ANSI_CLEAR_AND_HOME "\033[2J\033[H"
#define ANSI_HIDE_CURSOR "\033[?25l"
#define ANSI_SHOW_CURSOR "\033[?25h"

#define SINK_DEPTH 4
#define IDLE_HOOKS_MAX 4

struct sink {
    void (*put)(char c, void *context);
    void (*clear)(void *context);
    void *context;
};

static struct sink sinks[SINK_DEPTH];
static unsigned sink_count;
static bool graphical;
static void (*idle_hooks[IDLE_HOOKS_MAX])(void);
static unsigned idle_hook_count;
static void (*crash_hook)(void);

static long serial_row = -1;
static long serial_column = -1;
static int serial_attribute = -1;

/* ANSI color number (0-7) of each VGA color number (0-7); colors 8-15 are the bright variants. */
static const uint8_t vga_to_ansi[8] = { 0, 4, 2, 6, 1, 5, 3, 7 };

static void serial_puts(const char *string)
{
    while (*string != '\0')
        uart16550_write(*string++);
}

static void serial_put_number(size_t value)
{
    char digits[20];
    size_t count = 0;

    do {
        digits[count++] = (char)('0' + value % 10);
        value /= 10;
    } while (value != 0);
    while (count > 0)
        uart16550_write(digits[--count]);
}

static void serial_forget_position(void)
{
    serial_row = -1;
    serial_column = -1;
    serial_attribute = -1;
}

static void serial_move(size_t row, size_t column)
{
    if (serial_row == (long)row && serial_column == (long)column)
        return;
    serial_puts("\033[");
    serial_put_number(row + 1);
    uart16550_write(';');
    serial_put_number(column + 1);
    uart16550_write('H');
    serial_row = (long)row;
    serial_column = (long)column;
}

static void serial_set_attribute(uint8_t attribute)
{
    uint8_t foreground = attribute & 0x0F;
    uint8_t background = (uint8_t)((attribute >> 4) & 0x07);

    if (serial_attribute == attribute)
        return;
    serial_puts("\033[0;");
    serial_put_number((foreground >= 8 ? 90U : 30U) + vga_to_ansi[foreground & 7]);
    uart16550_write(';');
    serial_put_number(40U + vga_to_ansi[background]);
    uart16550_write('m');
    serial_attribute = attribute;
}

static void serial_put_glyph(uint8_t c)
{
    char encoded[4];
    size_t length = cp437_encode_utf8(c, encoded);

    for (size_t i = 0; i < length; i++)
        uart16550_write(encoded[i]);
}

void console_init(void)
{
    vga_text_init();
    uart16550_init();
    vga_text_clear();
}

size_t console_rows(void)
{
    return vga_text_rows();
}

void console_push_sink(void (*put)(char c, void *context), void (*clear)(void *context), void *context)
{
    if (sink_count == SINK_DEPTH)
        return;
    sinks[sink_count].put = put;
    sinks[sink_count].clear = clear;
    sinks[sink_count].context = context;
    sink_count++;
}

void console_pop_sink(void)
{
    if (sink_count > 0)
        sink_count--;
}

void console_set_graphical(bool value)
{
    graphical = value;
}

bool console_is_graphical(void)
{
    return graphical;
}

void console_add_idle_hook(void (*hook)(void))
{
    if (idle_hook_count < IDLE_HOOKS_MAX)
        idle_hooks[idle_hook_count++] = hook;
}

void console_run_idle_hooks(void)
{
    for (unsigned i = 0; i < idle_hook_count; i++)
        idle_hooks[i]();
}

void console_set_crash_hook(void (*hook)(void))
{
    crash_hook = hook;
}

void console_prepare_for_crash(void)
{
    sink_count = 0;
    if (crash_hook != NULL)
        crash_hook();
    graphical = false;
}

void console_putc(char c)
{
    if (sink_count > 0) {
        const struct sink *top = &sinks[sink_count - 1];

        top->put(c, top->context);
        return;
    }

    if (serial_attribute != -1) {
        serial_puts(ANSI_RESET_ATTRIBUTES);
    }
    serial_forget_position();
    if (c == '\n') {
        uart16550_write('\r');
        uart16550_write('\n');
    } else if (c == '\b') {
        uart16550_write('\b');
        uart16550_write(' ');
        uart16550_write('\b');
    } else {
        uart16550_write(c);
    }
    vga_text_putc(c);
}

void console_puts(const char *string)
{
    while (*string != '\0')
        console_putc(*string++);
}

void console_clear(void)
{
    if (sink_count > 0) {
        const struct sink *top = &sinks[sink_count - 1];

        if (top->clear != NULL)
            top->clear(top->context);
        return;
    }
    serial_puts(ANSI_RESET_ATTRIBUTES ANSI_SHOW_CURSOR ANSI_CLEAR_AND_HOME);
    serial_forget_position();
    vga_text_clear();
}

void console_tui_begin(void)
{
    if (!graphical)
        console_clear();
}

void console_tui_end(void)
{
    if (!graphical)
        console_clear();
}

void console_put_cell(size_t row, size_t column, uint8_t c, uint8_t attribute)
{
    if (graphical)
        return;
    vga_text_put_cell(row, column, c, attribute);

    serial_move(row, column);
    serial_set_attribute(attribute);
    serial_put_glyph(c);
    if (column + 1 < CONSOLE_COLUMNS)
        serial_column++;
    else
        serial_forget_position();
}

void console_set_cursor(size_t row, size_t column)
{
    if (graphical)
        return;
    vga_text_set_cursor(row, column);
    serial_puts(ANSI_SHOW_CURSOR);
    serial_move(row, column);
}

void console_hide_cursor(void)
{
    if (graphical)
        return;
    vga_text_hide_cursor();
    serial_puts(ANSI_HIDE_CURSOR);
}

static int serial_wait_byte(unsigned timeout_ms)
{
    for (unsigned waited = 0; waited <= timeout_ms; waited++) {
        int byte = uart16550_read();

        if (byte >= 0)
            return byte;
        timer_delay_ms(1);
    }
    return -1;
}

static int serial_decode_escape(void)
{
    int introducer = serial_wait_byte(ESCAPE_FOLLOW_TIMEOUT_MS);
    unsigned number = 0;

    if (introducer < 0)
        return KEY_ESCAPE;
    if (introducer != '[' && introducer != 'O')
        return -1;

    for (;;) {
        int byte = serial_wait_byte(ESCAPE_FOLLOW_TIMEOUT_MS);

        if (byte < 0)
            return -1;
        if (byte >= '0' && byte <= '9') {
            number = number * 10 + (unsigned)(byte - '0');
            continue;
        }
        if (byte == ';') {
            number = 0;
            continue;
        }

        switch (byte) {
        case 'A':
            return KEY_UP;
        case 'B':
            return KEY_DOWN;
        case 'C':
            return KEY_RIGHT;
        case 'D':
            return KEY_LEFT;
        case 'H':
            return KEY_HOME;
        case 'F':
            return KEY_END;
        case '~':
            switch (number) {
            case 1:
            case 7:
                return KEY_HOME;
            case 3:
                return KEY_DELETE;
            case 4:
            case 8:
                return KEY_END;
            case 5:
                return KEY_PAGE_UP;
            case 6:
                return KEY_PAGE_DOWN;
            default:
                return -1;
            }
        default:
            return -1;
        }
    }
}

static int serial_decode(int byte)
{
    if (byte == '\r')
        return KEY_ENTER;
    if (byte == ASCII_DELETE)
        return KEY_BACKSPACE;
    if (byte == KEY_ESCAPE)
        return serial_decode_escape();
    return byte;
}

int console_poll_key(void)
{
    int byte = uart16550_read();

    if (byte >= 0)
        return serial_decode(byte);
    return ps2_keyboard_read();
}

int console_read_key(void)
{
    for (;;) {
        int key = console_poll_key();

        if (key >= 0)
            return key;
        console_run_idle_hooks();
        timer_wait_for_tick();
    }
}
