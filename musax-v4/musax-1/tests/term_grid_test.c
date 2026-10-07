#include <string.h>
#include <musax/gui/term_grid.h>
#include "check.h"

static void put_string(struct term_grid *grid, const char *text)
{
    for (; *text != '\0'; text++)
        term_grid_putc(grid, *text);
}

static int row_equals(const struct term_grid *grid, int row, const char *text)
{
    char expected[TERM_COLUMNS];

    memset(expected, ' ', sizeof(expected));
    memcpy(expected, text, strlen(text));
    return memcmp(grid->cells[row], expected, TERM_COLUMNS) == 0;
}

int main(void)
{
    static struct term_grid grid;

    term_grid_clear(&grid);
    CHECK(grid.row == 0 && grid.column == 0 && row_equals(&grid, 0, ""));

    put_string(&grid, "hello");
    CHECK(row_equals(&grid, 0, "hello") && grid.column == 5);
    term_grid_putc(&grid, '\n');
    CHECK(grid.row == 1 && grid.column == 0);
    put_string(&grid, "ab");
    term_grid_putc(&grid, '\b');
    CHECK(grid.column == 1 && row_equals(&grid, 1, "a"));
    put_string(&grid, "c\r");
    CHECK(grid.column == 0 && row_equals(&grid, 1, "ac"));

    term_grid_putc(&grid, '\t');
    CHECK(grid.column == 8);
    term_grid_putc(&grid, '\t');
    CHECK(grid.column == 16);
    term_grid_putc(&grid, '\x01');
    term_grid_putc(&grid, '\x7f');
    CHECK(grid.column == 16);

    term_grid_clear(&grid);
    term_grid_putc(&grid, '\b');
    CHECK(grid.row == 0 && grid.column == 0);

    for (int i = 0; i < TERM_COLUMNS; i++)
        term_grid_putc(&grid, 'x');
    CHECK(grid.row == 1 && grid.column == 0);
    term_grid_putc(&grid, '\b');
    CHECK(grid.row == 0 && grid.column == TERM_COLUMNS - 1 && grid.cells[0][TERM_COLUMNS - 1] == ' ');

    term_grid_clear(&grid);
    for (int line = 0; line < TERM_ROWS + 5; line++) {
        char label[8] = { 'L', (char)('a' + line % 26), '\n', 0 };

        put_string(&grid, label);
    }
    CHECK(grid.row == TERM_ROWS - 1);
    CHECK(row_equals(&grid, 0, "Lg"));
    CHECK(row_equals(&grid, TERM_ROWS - 3, "Lc"));
    CHECK(row_equals(&grid, TERM_ROWS - 2, "Ld"));
    CHECK(row_equals(&grid, TERM_ROWS - 1, ""));

    CHECK_REPORT("term_grid");
}
