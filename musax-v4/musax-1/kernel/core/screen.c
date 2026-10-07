#include <stddef.h>
#include <stdint.h>
#include <musax/console.h>
#include <musax/screen.h>

struct cell {
    uint8_t c;
    uint8_t attribute;
};

static struct cell frame[CONSOLE_MAX_ROWS][CONSOLE_COLUMNS];
static struct cell shown[CONSOLE_MAX_ROWS][CONSOLE_COLUMNS];

static void blank(struct cell cells[CONSOLE_MAX_ROWS][CONSOLE_COLUMNS], uint8_t attribute)
{
    for (size_t row = 0; row < CONSOLE_MAX_ROWS; row++) {
        for (size_t column = 0; column < CONSOLE_COLUMNS; column++) {
            cells[row][column].c = ' ';
            cells[row][column].attribute = attribute;
        }
    }
}

static void flush_cells(void)
{
    size_t rows = console_rows();

    for (size_t row = 0; row < rows; row++) {
        for (size_t column = 0; column < CONSOLE_COLUMNS; column++) {
            const struct cell *want = &frame[row][column];
            struct cell *have = &shown[row][column];

            if (want->c == have->c && want->attribute == have->attribute)
                continue;
            console_put_cell(row, column, want->c, want->attribute);
            *have = *want;
        }
    }
}

void screen_begin(void)
{
    console_tui_begin();
    blank(frame, CONSOLE_ATTRIBUTE_NORMAL);
    blank(shown, CONSOLE_ATTRIBUTE_NORMAL);
}

void screen_end(void)
{
    console_tui_end();
}

void screen_clear(uint8_t attribute)
{
    blank(frame, attribute);
}

void screen_put(size_t row, size_t column, uint8_t c, uint8_t attribute)
{
    if (row >= CONSOLE_MAX_ROWS || column >= CONSOLE_COLUMNS)
        return;
    frame[row][column].c = c;
    frame[row][column].attribute = attribute;
}

void screen_fill_row(size_t row, uint8_t c, uint8_t attribute)
{
    for (size_t column = 0; column < CONSOLE_COLUMNS; column++)
        screen_put(row, column, c, attribute);
}

void screen_write(size_t row, size_t column, const char *text, size_t length, uint8_t attribute)
{
    for (size_t i = 0; i < length; i++)
        screen_put(row, column + i, (uint8_t)text[i], attribute);
}

void screen_flush(size_t cursor_row, size_t cursor_column)
{
    flush_cells();
    console_set_cursor(cursor_row, cursor_column);
}

void screen_flush_hidden(void)
{
    flush_cells();
    console_hide_cursor();
}
