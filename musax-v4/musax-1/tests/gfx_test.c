#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <musax/gui/gfx.h>
#include "check.h"

#define W 64
#define H 48

static uint32_t pixels[W * H];
static struct gfx surface = { pixels, W, H, W };

static void clear(color_t color)
{
    for (int i = 0; i < W * H; i++)
        pixels[i] = color;
}

static color_t at(int x, int y)
{
    return pixels[y * W + x];
}

static int count_not(color_t color)
{
    int count = 0;

    for (int i = 0; i < W * H; i++)
        count += pixels[i] != color;
    return count;
}

int main(void)
{
    static uint8_t font[256 * GFX_FONT_HEIGHT];
    uint32_t other[W * H];
    struct gfx destination = { other, W, H, W };

    CHECK(!gfx_set_font(font));
    font['A' * GFX_FONT_HEIGHT + 0] = 0x81;
    font['A' * GFX_FONT_HEIGHT + 15] = 0xFF;
    CHECK(gfx_set_font(font));

    clear(0);
    gfx_fill_rect(&surface, (struct rect){ 10, 10, 5, 4 }, 0xFFFFFF);
    CHECK(count_not(0) == 20);
    CHECK(at(10, 10) == 0xFFFFFF && at(14, 13) == 0xFFFFFF && at(15, 13) == 0 && at(14, 14) == 0);

    clear(0);
    gfx_fill_rect(&surface, (struct rect){ -5, -5, 10, 10 }, 1);
    CHECK(count_not(0) == 25);
    gfx_fill_rect(&surface, (struct rect){ W - 3, H - 3, 100, 100 }, 2);
    CHECK(at(W - 1, H - 1) == 2 && at(W - 4, H - 4) == 0);
    gfx_fill_rect(&surface, (struct rect){ 200, 200, 5, 5 }, 3);
    gfx_fill_rect(&surface, (struct rect){ 5, 5, 0, 5 }, 3);
    gfx_fill_rect(&surface, (struct rect){ 5, 5, -2, 5 }, 3);

    clear(0);
    gfx_rect_outline(&surface, (struct rect){ 4, 4, 10, 6 }, 9);
    CHECK(count_not(0) == 2 * 10 + 2 * 4);
    CHECK(at(4, 4) == 9 && at(13, 9) == 9 && at(5, 5) == 0);

    clear(RGB(0, 0, 0));
    gfx_fill_rect_alpha(&surface, (struct rect){ 0, 0, 4, 4 }, RGB(200, 100, 50), 128);
    CHECK(at(1, 1) == RGB(100, 50, 25));
    CHECK(color_mix(RGB(0, 0, 0), RGB(255, 255, 255), 0) == RGB(0, 0, 0));
    CHECK(color_mix(RGB(10, 20, 30), RGB(250, 250, 250), 256) == RGB(250, 250, 250));

    clear(0);
    gfx_fill_gradient(&surface, (struct rect){ 0, 0, W, 11 }, RGB(0, 0, 0), RGB(100, 100, 100));
    CHECK(at(0, 0) == RGB(0, 0, 0) && at(5, 10) == RGB(100, 100, 100) && at(7, 5) == RGB(50, 50, 50));
    CHECK(at(0, 11) == 0);

    clear(0);
    gfx_line(&surface, 2, 2, 12, 2, 7);
    CHECK(count_not(0) == 11 && at(2, 2) == 7 && at(12, 2) == 7);
    clear(0);
    gfx_line(&surface, 3, 3, 3, 13, 7);
    CHECK(count_not(0) == 11);
    clear(0);
    gfx_line(&surface, 5, 5, 15, 15, 7);
    CHECK(count_not(0) == 11 && at(10, 10) == 7);
    clear(0);
    gfx_line(&surface, 15, 5, 5, 15, 7);
    CHECK(count_not(0) == 11 && at(10, 10) == 7);
    clear(0);
    gfx_line(&surface, -10, 5, 200, 5, 7);
    CHECK(count_not(0) == W);
    gfx_line(&surface, 4, 4, 4, 4, 8);
    CHECK(at(4, 4) == 8);

    clear(0);
    gfx_draw_char(&surface, 8, 8, 'A', 5);
    CHECK(at(8, 8) == 5 && at(15, 8) == 5 && at(9, 8) == 0 && at(8, 23) == 5 && at(15, 23) == 5 && at(12, 23) == 5);
    CHECK(count_not(0) == 2 + 8);
    clear(0);
    gfx_draw_text(&surface, 0, 0, "AA", 5);
    CHECK(at(0, 0) == 5 && at(8, 0) == 5 && at(16, 0) == 0);
    CHECK(gfx_text_width("abc") == 3 * GFX_FONT_WIDTH && gfx_text_width("") == 0);
    clear(0);
    gfx_draw_char(&surface, W - 4, H - 4, 'A', 5);
    gfx_draw_char(&surface, -3, -3, 'A', 5);

    clear(0x111111);
    memset(other, 0, sizeof(other));
    gfx_copy_rect(&destination, &surface, (struct rect){ 3, 4, 10, 6 });
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            int inside = x >= 3 && x < 13 && y >= 4 && y < 10;

            CHECK(other[y * W + x] == (inside ? 0x111111U : 0U));
        }
    }
    gfx_copy_rect(&destination, &surface, (struct rect){ -10, -10, 1000, 1000 });
    CHECK(other[0] == 0x111111 && other[W * H - 1] == 0x111111);

    clear(0x222222);
    gfx_draw_cursor(&surface, 10, 10);
    CHECK(at(10, 10) == 0 && at(11, 10) == 0x222222 && at(11, 11) == 0 && at(11, 12) == 0xFFFFFF);
    gfx_draw_cursor(&surface, W - 2, H - 2);

    {
        struct rect a = { 0, 0, 10, 10 };
        struct rect b = { 5, 5, 10, 10 };
        struct rect c = { 10, 0, 5, 5 };
        struct rect u = rect_union(&a, &b);

        CHECK(rect_intersects(&a, &b) && !rect_intersects(&a, &c));
        CHECK(u.x == 0 && u.y == 0 && u.w == 15 && u.h == 15);
        CHECK(rect_contains(&a, 0, 0) && rect_contains(&a, 9, 9) && !rect_contains(&a, 10, 9));
    }

    CHECK_REPORT("gfx");
}
