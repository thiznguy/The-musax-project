#include <string.h>
#include <musax/gui/term_grid.h>

#define TAB_WIDTH 8

static void clear_row(struct term_grid *grid, int row)
{
    memset(grid->cells[row], ' ', TERM_COLUMNS);
}

static void scroll_if_needed(struct term_grid *grid)
{
    if (grid->row < TERM_ROWS)
        return;
    memmove(grid->cells[0], grid->cells[1], (TERM_ROWS - 1) * TERM_COLUMNS);
    clear_row(grid, TERM_ROWS - 1);
    grid->row = TERM_ROWS - 1;
}

void term_grid_clear(struct term_grid *grid)
{
    for (int row = 0; row < TERM_ROWS; row++)
        clear_row(grid, row);
    grid->row = 0;
    grid->column = 0;
}

void term_grid_putc(struct term_grid *grid, char c)
{
    switch (c) {
    case '\n':
        grid->column = 0;
        grid->row++;
        scroll_if_needed(grid);
        return;
    case '\r':
        grid->column = 0;
        return;
    case '\b':
        if (grid->column > 0) {
            grid->column--;
        } else if (grid->row > 0) {
            grid->row--;
            grid->column = TERM_COLUMNS - 1;
        }
        grid->cells[grid->row][grid->column] = ' ';
        return;
    case '\t':
        do {
            term_grid_putc(grid, ' ');
        } while (grid->column % TAB_WIDTH != 0);
        return;
    default:
        break;
    }

    if (c < 0x20 || c > 0x7E)
        return;
    grid->cells[grid->row][grid->column] = c;
    grid->column++;
    if (grid->column >= TERM_COLUMNS) {
        grid->column = 0;
        grid->row++;
        scroll_if_needed(grid);
    }
}
