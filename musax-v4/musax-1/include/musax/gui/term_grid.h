#ifndef MUSAX_GUI_TERM_GRID_H
#define MUSAX_GUI_TERM_GRID_H

#define TERM_COLUMNS 80
#define TERM_ROWS 25

/* Character cells of a terminal window plus the cursor position. */
struct term_grid {
    char cells[TERM_ROWS][TERM_COLUMNS];
    int row;
    int column;
};

void term_grid_clear(struct term_grid *grid);

/* Interprets one output character: printable text, newline, carriage return, backspace and tab. */
void term_grid_putc(struct term_grid *grid, char c);

#endif
