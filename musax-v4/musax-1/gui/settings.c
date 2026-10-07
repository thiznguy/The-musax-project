#include <stdio.h>
#include <musax/gui/apps.h>
#include <musax/gui/desktop.h>
#include <musax/gui/gfx.h>
#include <musax/gui/theme.h>
#include <musax/gui/wm.h>

#define SETTINGS_X 300
#define SETTINGS_Y 140
#define CLIENT_WIDTH 360
#define CLIENT_HEIGHT 240
#define GRID_LEFT 16
#define GRID_TOP 60
#define COLUMNS 4
#define CELL_WIDTH 84
#define CELL_HEIGHT 88
#define SWATCH_WIDTH 76
#define SWATCH_HEIGHT 56

static struct rect swatch_rect(const struct rect *client, size_t index)
{
    struct rect swatch = { client->x + GRID_LEFT + (int)(index % COLUMNS) * CELL_WIDTH,
                           client->y + GRID_TOP + (int)(index / COLUMNS) * CELL_HEIGHT, SWATCH_WIDTH, SWATCH_HEIGHT };

    return swatch;
}

static void settings_draw(struct window *window, struct gfx *gfx, struct rect client)
{
    size_t selected = desktop_theme();

    (void)window;
    gfx_fill_rect(gfx, client, RGB(36, 40, 52));
    gfx_draw_text(gfx, client.x + 16, client.y + 12, "Background", RGB(255, 255, 255));
    gfx_draw_text(gfx, client.x + 16, client.y + 32, "Click a color to apply it.", RGB(150, 158, 176));

    for (size_t i = 0; i < theme_count(); i++) {
        const struct theme *theme = theme_at(i);
        struct rect swatch = swatch_rect(&client, i);
        int label_x = swatch.x + (SWATCH_WIDTH - gfx_text_width(theme->name)) / 2;

        gfx_fill_gradient(gfx, swatch, theme->top, theme->bottom);
        gfx_rect_outline(gfx, swatch, RGB(16, 20, 30));
        if (i == selected) {
            struct rect outer = { swatch.x - 3, swatch.y - 3, swatch.w + 6, swatch.h + 6 };
            struct rect inner = { swatch.x - 2, swatch.y - 2, swatch.w + 4, swatch.h + 4 };

            gfx_rect_outline(gfx, outer, RGB(255, 255, 255));
            gfx_rect_outline(gfx, inner, RGB(255, 255, 255));
        }
        gfx_draw_text(gfx, label_x, swatch.y + SWATCH_HEIGHT + 6, theme->name,
                      i == selected ? RGB(255, 255, 255) : RGB(170, 178, 196));
    }
}

static void settings_click(struct window *window, int x, int y)
{
    struct rect origin = { 0, 0, 0, 0 };

    (void)window;
    for (size_t i = 0; i < theme_count(); i++) {
        struct rect swatch = swatch_rect(&origin, i);

        if (rect_contains(&swatch, x, y)) {
            desktop_set_theme(i);
            return;
        }
    }
}

static const struct window_ops settings_ops = {
    .draw = settings_draw,
    .click = settings_click,
};

struct window *settings_open(void)
{
    struct window *window = wm_find(&settings_ops);

    if (window != NULL) {
        wm_focus(window);
        return window;
    }
    return wm_open("Settings", SETTINGS_X, SETTINGS_Y, CLIENT_WIDTH, CLIENT_HEIGHT, &settings_ops, NULL);
}
