#ifndef MUSAX_GUI_GFX_H
#define MUSAX_GUI_GFX_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define GFX_FONT_WIDTH 8
#define GFX_FONT_HEIGHT 16

/* 0x00RRGGBB */
typedef uint32_t color_t;

#define RGB(r, g, b) ((color_t)(((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(b)))

struct rect {
    int x;
    int y;
    int w;
    int h;
};

/* A drawing target of 32-bit pixels. */
struct gfx {
    uint32_t *pixels;
    int width;
    int height;
    int pitch_pixels;
};

/* Installs the 256-glyph, 16-bytes-per-glyph font the drawing functions use; returns false when it holds no glyphs. */
bool gfx_set_font(const uint8_t *font);

bool rect_contains(const struct rect *rect, int x, int y);
bool rect_intersects(const struct rect *a, const struct rect *b);
struct rect rect_union(const struct rect *a, const struct rect *b);

void gfx_fill_rect(struct gfx *gfx, struct rect rect, color_t color);
void gfx_fill_rect_alpha(struct gfx *gfx, struct rect rect, color_t color, unsigned alpha);
void gfx_rect_outline(struct gfx *gfx, struct rect rect, color_t color);
void gfx_fill_gradient(struct gfx *gfx, struct rect rect, color_t top, color_t bottom);
void gfx_line(struct gfx *gfx, int x0, int y0, int x1, int y1, color_t color);

void gfx_draw_char(struct gfx *gfx, int x, int y, uint8_t c, color_t foreground);
void gfx_draw_text(struct gfx *gfx, int x, int y, const char *text, color_t foreground);
int gfx_text_width(const char *text);

/* Copies a rectangle of one surface onto another surface at the same coordinates. */
void gfx_copy_rect(struct gfx *destination, const struct gfx *source, struct rect rect);

/* Mouse pointer sprite. */
#define CURSOR_WIDTH 12
#define CURSOR_HEIGHT 19
void gfx_draw_cursor(struct gfx *gfx, int x, int y);

color_t color_mix(color_t from, color_t to, unsigned amount_of_256);

#endif
