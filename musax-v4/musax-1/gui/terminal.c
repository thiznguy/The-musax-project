#include <stdbool.h>
#include <musax/gui/apps.h>
#include <musax/gui/gfx.h>
#include <musax/gui/term_grid.h>
#include <musax/gui/wm.h>
#include <musax/console.h>
#include <musax/kprintf.h>
#include <musax/shell/music.h>
#include <musax/version.h>

#define TERMINAL_X 191
#define TERMINAL_Y 90

static struct term_grid grid;
static bool grid_ready;
static bool shell_started;

static const color_t background = RGB(12, 14, 20);
static const color_t foreground = RGB(214, 218, 226);

static void sink_put(char c, void *context)
{
    (void)context;
    term_grid_putc(&grid, c);
}

static void sink_clear(void *context)
{
    (void)context;
    term_grid_clear(&grid);
}

void terminal_attach_output(void)
{
    if (!grid_ready) {
        term_grid_clear(&grid);
        grid_ready = true;
    }
    console_push_sink(sink_put, sink_clear, NULL);
}

void terminal_detach_output(void)
{
    console_pop_sink();
}

static void terminal_draw(struct window *window, struct gfx *gfx, struct rect client)
{
    (void)window;
    gfx_fill_rect(gfx, client, background);

    for (int row = 0; row < TERM_ROWS; row++) {
        for (int column = 0; column < TERM_COLUMNS; column++) {
            char c = grid.cells[row][column];

            if (c != ' ')
                gfx_draw_char(gfx, client.x + column * GFX_FONT_WIDTH, client.y + row * GFX_FONT_HEIGHT,
                              (uint8_t)c, foreground);
        }
    }

    {
        struct rect cursor = { client.x + grid.column * GFX_FONT_WIDTH, client.y + grid.row * GFX_FONT_HEIGHT,
                               GFX_FONT_WIDTH, GFX_FONT_HEIGHT };
        char under = grid.cells[grid.row][grid.column];

        gfx_fill_rect(gfx, cursor, foreground);
        if (under != ' ')
            gfx_draw_char(gfx, cursor.x, cursor.y, (uint8_t)under, background);
    }
}

static void terminal_key(struct window *window, int key)
{
    (void)window;
    music_session_key(key);
}

static const struct window_ops terminal_ops = {
    .draw = terminal_draw,
    .key = terminal_key,
};

struct window *terminal_open(void)
{
    struct window *window = wm_find(&terminal_ops);

    if (window != NULL) {
        wm_focus(window);
        return window;
    }
    window = wm_open("Terminal", TERMINAL_X, TERMINAL_Y, TERM_COLUMNS * GFX_FONT_WIDTH, TERM_ROWS * GFX_FONT_HEIGHT,
                     &terminal_ops, NULL);
    if (window != NULL && !shell_started) {
        shell_started = true;
        kprintf("%s %s desktop terminal\n", MUSAX_NAME, MUSAX_VERSION);
        music_session_start();
    }
    return window;
}
