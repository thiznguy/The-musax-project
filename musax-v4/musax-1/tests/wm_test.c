#include <stdint.h>
#include <string.h>
#include <musax/gui/wm.h>
#include "check.h"

#define SCREEN_W 1024
#define SCREEN_H 768
#define TOP 28

static int draws;
static int clicks;
static int last_click_x;
static int last_click_y;
static int keys;
static int last_key;
static int closed_calls;

static void draw(struct window *window, struct gfx *gfx, struct rect client)
{
    (void)window;
    (void)gfx;
    (void)client;
    draws++;
}

static void click(struct window *window, int x, int y)
{
    (void)window;
    clicks++;
    last_click_x = x;
    last_click_y = y;
}

static void key(struct window *window, int value)
{
    (void)window;
    keys++;
    last_key = value;
}

static void closed(struct window *window)
{
    (void)window;
    closed_calls++;
}

static const struct window_ops full_ops = { draw, click, key, closed };
static const struct window_ops plain_ops = { .draw = draw };

static void test_open_close(void)
{
    struct window *a;
    struct window *b;
    struct window *windows[WM_MAX_WINDOWS + 1];
    struct rect client;
    struct rect button;

    wm_init(SCREEN_W, SCREEN_H, TOP);
    CHECK(wm_window_count() == 0 && wm_focused() == NULL);

    a = wm_open("first", 100, 100, 300, 200, &full_ops, NULL);
    CHECK(a != NULL && wm_focused() == a && wm_window_count() == 1);
    CHECK(a->frame.w == 302 && a->frame.h == 200 + WM_TITLE_HEIGHT + 1);
    client = wm_client_rect(a);
    CHECK(client.w == 300 && client.h == 200 && client.x == 101 && client.y == 100 + WM_TITLE_HEIGHT);
    button = wm_close_button_rect(a);
    CHECK(button.x + button.w + 6 == a->frame.x + a->frame.w && button.y == a->frame.y + 4);

    b = wm_open("second", 0, 0, 100, 100, &plain_ops, NULL);
    CHECK(b->frame.y == TOP);
    CHECK(wm_focused() == b && wm_find(&full_ops) == a && wm_find(&plain_ops) == b);
    wm_focus(a);
    CHECK(wm_focused() == a);

    closed_calls = 0;
    wm_close(a);
    CHECK(closed_calls == 1 && wm_window_count() == 1 && wm_focused() == b && !a->open);
    wm_close(a);
    CHECK(closed_calls == 1);

    wm_close_all();
    CHECK(wm_window_count() == 0);

    for (int i = 0; i < WM_MAX_WINDOWS; i++) {
        windows[i] = wm_open("w", 10 * i, 50, 50, 50, &plain_ops, NULL);
        CHECK(windows[i] != NULL);
    }
    CHECK(wm_open("overflow", 0, 50, 50, 50, &plain_ops, NULL) == NULL);
    wm_close(windows[3]);
    CHECK(wm_open("again", 0, 50, 50, 50, &plain_ops, NULL) != NULL);
    wm_close_all();
}

static void test_clicks_and_focus(void)
{
    struct window *back;
    struct window *front;
    struct rect button;
    struct rect client;

    wm_init(SCREEN_W, SCREEN_H, TOP);
    back = wm_open("back", 100, 100, 300, 200, &full_ops, NULL);
    front = wm_open("front", 200, 150, 300, 200, &full_ops, NULL);
    CHECK(wm_focused() == front);

    clicks = 0;
    client = wm_client_rect(back);
    CHECK(wm_mouse(client.x + 10, client.y + 10, true, false, true));
    CHECK(wm_focused() == back && clicks == 1 && last_click_x == 10 && last_click_y == 10);
    wm_mouse(client.x + 10, client.y + 10, false, true, false);

    CHECK(!wm_mouse(900, 700, true, false, true));
    CHECK(wm_focused() == back);
    wm_mouse(900, 700, false, true, false);

    clicks = 0;
    CHECK(wm_mouse(back->frame.x + 20, back->frame.y + 5, true, false, true));
    CHECK(clicks == 0);
    wm_mouse(back->frame.x + 20, back->frame.y + 5, false, true, false);

    CHECK(wm_close_button_at(front->frame.x + front->frame.w - 15, front->frame.y + 10) == NULL ||
          wm_close_button_at(front->frame.x + front->frame.w - 15, front->frame.y + 10) == front);
    button = wm_close_button_rect(front);
    CHECK(wm_close_button_at(button.x + 2, button.y + 2) == front);
    CHECK(wm_close_button_at(5, 500) == NULL);

    closed_calls = 0;
    wm_focus(front);
    CHECK(wm_mouse(button.x + 3, button.y + 3, true, false, true));
    CHECK(wm_window_count() == 1 && closed_calls == 1 && wm_focused() == back);
    wm_mouse(button.x + 3, button.y + 3, false, true, false);
    wm_close_all();
}

static void test_close_button_hidden_behind_other_window(void)
{
    struct window *back;
    struct window *front;
    struct rect back_button;

    wm_init(SCREEN_W, SCREEN_H, TOP);
    back = wm_open("back", 100, 100, 300, 200, &full_ops, NULL);
    front = wm_open("front", 100, 100, 400, 300, &full_ops, NULL);
    back_button = wm_close_button_rect(back);
    CHECK(back_button.x < front->frame.x + front->frame.w);
    CHECK(wm_close_button_at(back_button.x + 2, back_button.y + 2) == NULL ||
          wm_close_button_at(back_button.x + 2, back_button.y + 2) == front);
    wm_close_all();
}

static void test_drag(void)
{
    struct window *window;
    int start_x;
    int start_y;

    wm_init(SCREEN_W, SCREEN_H, TOP);
    window = wm_open("drag", 300, 200, 200, 100, &full_ops, NULL);
    start_x = window->frame.x;
    start_y = window->frame.y;

    CHECK(wm_mouse(start_x + 50, start_y + 10, true, false, true));
    CHECK(wm_mouse(start_x + 90, start_y + 60, false, false, true));
    CHECK(window->frame.x == start_x + 40 && window->frame.y == start_y + 50);
    CHECK(wm_mouse(start_x + 90, start_y + 60, false, true, false) == false);
    CHECK(!wm_mouse(0, 0, false, false, false));
    CHECK(window->frame.x == start_x + 40);

    wm_mouse(window->frame.x + 5, window->frame.y + 5, true, false, true);
    wm_mouse(window->frame.x + 5, -500, false, false, true);
    CHECK(window->frame.y == TOP);
    wm_mouse(5000, 5000, false, false, true);
    CHECK(window->frame.x <= SCREEN_W - 80 && window->frame.y <= SCREEN_H - WM_TITLE_HEIGHT);
    wm_mouse(-5000, 300, false, false, true);
    CHECK(window->frame.x + window->frame.w >= 80);
    wm_mouse(0, 0, false, true, false);
    wm_close_all();

    window = wm_open("drag2", 300, 200, 200, 100, &full_ops, NULL);
    wm_mouse(window->frame.x + 20, window->frame.y + 10, true, false, true);
    wm_close(window);
    CHECK(!wm_mouse(10, 10, false, false, true));
}

static void test_keys_and_draw(void)
{
    struct window *plain;

    wm_init(SCREEN_W, SCREEN_H, TOP);
    wm_key(65);
    wm_open("full", 100, 100, 100, 100, &full_ops, NULL);
    keys = 0;
    wm_key(66);
    CHECK(keys == 1 && last_key == 66);
    plain = wm_open("plain", 200, 200, 100, 100, &plain_ops, NULL);
    wm_key(67);
    CHECK(keys == 1 && plain != NULL);

    {
        static uint32_t pixels[SCREEN_W * SCREEN_H];
        struct gfx gfx = { pixels, SCREEN_W, SCREEN_H, SCREEN_W };
        int title_changed = 0;

        draws = 0;
        wm_draw(&gfx);
        CHECK(draws == 2);
        for (int x = 100; x < 200; x++)
            title_changed += pixels[(100 + 5) * SCREEN_W + x] != 0;
        CHECK(title_changed > 50);
    }
    wm_close_all();
}

int main(void)
{
    test_open_close();
    test_clicks_and_focus();
    test_close_button_hidden_behind_other_window();
    test_drag();
    test_keys_and_draw();
    CHECK_REPORT("wm");
}
