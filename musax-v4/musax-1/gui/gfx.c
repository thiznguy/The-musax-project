#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <musax/gui/gfx.h>

#define GLYPH_COUNT 256

static uint8_t font[GLYPH_COUNT * GFX_FONT_HEIGHT];

/* Pointer shape: X is the outline, . is the fill, a space is transparent. */
static const char *const cursor_shape[CURSOR_HEIGHT] = {
    "X           ", "XX          ", "X.X         ", "X..X        ", "X...X       ", "X....X      ", "X.....X     ",
    "X......X    ", "X.......X   ", "X........X  ", "X.....XXXXX ", "X..X..X     ", "X.X X..X    ", "XX  X..X    ",
    "X    X..X   ", "     X..X   ", "      X..X  ", "      X..X  ", "       XX   ",
};

bool gfx_set_font(const uint8_t *source)
{
    uint32_t sum = 0;

    for (size_t i = 0; i < sizeof(font); i++)
        sum |= source[i];
    if (sum == 0)
        return false;
    memcpy(font, source, sizeof(font));
    return true;
}

bool rect_contains(const struct rect *rect, int x, int y)
{
    return x >= rect->x && y >= rect->y && x < rect->x + rect->w && y < rect->y + rect->h;
}

bool rect_intersects(const struct rect *a, const struct rect *b)
{
    return a->x < b->x + b->w && b->x < a->x + a->w && a->y < b->y + b->h && b->y < a->y + a->h;
}

struct rect rect_union(const struct rect *a, const struct rect *b)
{
    int left = a->x < b->x ? a->x : b->x;
    int top = a->y < b->y ? a->y : b->y;
    int right = a->x + a->w > b->x + b->w ? a->x + a->w : b->x + b->w;
    int bottom = a->y + a->h > b->y + b->h ? a->y + a->h : b->y + b->h;
    struct rect result = { left, top, right - left, bottom - top };

    return result;
}

static bool clip(const struct gfx *gfx, struct rect *rect)
{
    int right = rect->x + rect->w;
    int bottom = rect->y + rect->h;

    if (rect->x < 0)
        rect->x = 0;
    if (rect->y < 0)
        rect->y = 0;
    if (right > gfx->width)
        right = gfx->width;
    if (bottom > gfx->height)
        bottom = gfx->height;
    rect->w = right - rect->x;
    rect->h = bottom - rect->y;
    return rect->w > 0 && rect->h > 0;
}

static void put_pixel(struct gfx *gfx, int x, int y, color_t color)
{
    if (x >= 0 && y >= 0 && x < gfx->width && y < gfx->height)
        gfx->pixels[(size_t)y * gfx->pitch_pixels + x] = color;
}

color_t color_mix(color_t from, color_t to, unsigned amount)
{
    unsigned red = (((from >> 16) & 0xFF) * (256 - amount) + ((to >> 16) & 0xFF) * amount) / 256;
    unsigned green = (((from >> 8) & 0xFF) * (256 - amount) + ((to >> 8) & 0xFF) * amount) / 256;
    unsigned blue = ((from & 0xFF) * (256 - amount) + (to & 0xFF) * amount) / 256;

    return RGB(red, green, blue);
}

void gfx_fill_rect(struct gfx *gfx, struct rect rect, color_t color)
{
    if (!clip(gfx, &rect))
        return;
    for (int row = 0; row < rect.h; row++) {
        uint32_t *line = gfx->pixels + (size_t)(rect.y + row) * gfx->pitch_pixels + rect.x;

        for (int column = 0; column < rect.w; column++)
            line[column] = color;
    }
}

void gfx_fill_rect_alpha(struct gfx *gfx, struct rect rect, color_t color, unsigned alpha)
{
    if (!clip(gfx, &rect))
        return;
    for (int row = 0; row < rect.h; row++) {
        uint32_t *line = gfx->pixels + (size_t)(rect.y + row) * gfx->pitch_pixels + rect.x;

        for (int column = 0; column < rect.w; column++)
            line[column] = color_mix(line[column], color, alpha);
    }
}

void gfx_rect_outline(struct gfx *gfx, struct rect rect, color_t color)
{
    struct rect top = { rect.x, rect.y, rect.w, 1 };
    struct rect bottom = { rect.x, rect.y + rect.h - 1, rect.w, 1 };
    struct rect left = { rect.x, rect.y, 1, rect.h };
    struct rect right = { rect.x + rect.w - 1, rect.y, 1, rect.h };

    gfx_fill_rect(gfx, top, color);
    gfx_fill_rect(gfx, bottom, color);
    gfx_fill_rect(gfx, left, color);
    gfx_fill_rect(gfx, right, color);
}

void gfx_fill_gradient(struct gfx *gfx, struct rect rect, color_t top, color_t bottom)
{
    for (int row = 0; row < rect.h; row++) {
        unsigned amount = rect.h > 1 ? (unsigned)(row * 256 / (rect.h - 1)) : 0;
        struct rect line = { rect.x, rect.y + row, rect.w, 1 };

        gfx_fill_rect(gfx, line, color_mix(top, bottom, amount));
    }
}

void gfx_line(struct gfx *gfx, int x0, int y0, int x1, int y1, color_t color)
{
    int dx = x1 > x0 ? x1 - x0 : x0 - x1;
    int dy = y1 > y0 ? y0 - y1 : y1 - y0;
    int step_x = x0 < x1 ? 1 : -1;
    int step_y = y0 < y1 ? 1 : -1;
    int error = dx + dy;

    for (;;) {
        int doubled;

        put_pixel(gfx, x0, y0, color);
        if (x0 == x1 && y0 == y1)
            return;
        doubled = 2 * error;
        if (doubled >= dy) {
            error += dy;
            x0 += step_x;
        }
        if (doubled <= dx) {
            error += dx;
            y0 += step_y;
        }
    }
}

void gfx_draw_char(struct gfx *gfx, int x, int y, uint8_t c, color_t foreground)
{
    const uint8_t *glyph = &font[(size_t)c * GFX_FONT_HEIGHT];

    for (int row = 0; row < GFX_FONT_HEIGHT; row++) {
        uint8_t bits = glyph[row];

        for (int column = 0; column < GFX_FONT_WIDTH; column++) {
            if (bits & (0x80 >> column))
                put_pixel(gfx, x + column, y + row, foreground);
        }
    }
}

void gfx_draw_text(struct gfx *gfx, int x, int y, const char *text, color_t foreground)
{
    for (; *text != '\0'; text++, x += GFX_FONT_WIDTH)
        gfx_draw_char(gfx, x, y, (uint8_t)*text, foreground);
}

int gfx_text_width(const char *text)
{
    return (int)strlen(text) * GFX_FONT_WIDTH;
}

void gfx_copy_rect(struct gfx *destination, const struct gfx *source, struct rect rect)
{
    if (!clip(destination, &rect))
        return;
    for (int row = 0; row < rect.h; row++) {
        const uint32_t *from = source->pixels + (size_t)(rect.y + row) * source->pitch_pixels + rect.x;
        uint32_t *to = destination->pixels + (size_t)(rect.y + row) * destination->pitch_pixels + rect.x;
        size_t count = (size_t)rect.w;

        __asm__ volatile("rep movsl" : "+D"(to), "+S"(from), "+c"(count) : : "memory");
    }
}

void gfx_draw_cursor(struct gfx *gfx, int x, int y)
{
    for (int row = 0; row < CURSOR_HEIGHT; row++) {
        for (int column = 0; column < CURSOR_WIDTH; column++) {
            char shape = cursor_shape[row][column];

            if (shape == 'X')
                put_pixel(gfx, x + column, y + row, RGB(0, 0, 0));
            else if (shape == '.')
                put_pixel(gfx, x + column, y + row, RGB(255, 255, 255));
        }
    }
}
