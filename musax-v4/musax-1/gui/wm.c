#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <musax/gui/gfx.h>
#include <musax/gui/wm.h>

#define SHADOW_OFFSET 6
#define SHADOW_ALPHA 90
#define MIN_VISIBLE_WIDTH 80

static struct window pool[WM_MAX_WINDOWS];
static struct window *order[WM_MAX_WINDOWS];
static size_t count;
static int screen_width;
static int screen_height;
static int top_limit;

static struct {
    bool active;
    struct window *window;
    int offset_x;
    int offset_y;
} drag;

static const color_t border_color = RGB(16, 20, 30);
static const color_t body_color = RGB(30, 34, 44);

void wm_init(int width, int height, int limit)
{
    memset(pool, 0, sizeof(pool));
    memset(order, 0, sizeof(order));
    count = 0;
    screen_width = width;
    screen_height = height;
    top_limit = limit;
    drag.active = false;
}

size_t wm_window_count(void)
{
    return count;
}

struct window *wm_focused(void)
{
    return count > 0 ? order[count - 1] : NULL;
}

struct rect wm_client_rect(const struct window *window)
{
    struct rect client = { window->frame.x + 1, window->frame.y + WM_TITLE_HEIGHT, window->frame.w - 2,
                           window->frame.h - WM_TITLE_HEIGHT - 1 };

    return client;
}

struct rect wm_close_button_rect(const struct window *window)
{
    struct rect button = { window->frame.x + window->frame.w - WM_CLOSE_SIZE - 6, window->frame.y + 4,
                           WM_CLOSE_SIZE, WM_CLOSE_SIZE };

    return button;
}

void wm_focus(struct window *window)
{
    size_t index = 0;

    while (index < count && order[index] != window)
        index++;
    if (index == count)
        return;
    for (; index + 1 < count; index++)
        order[index] = order[index + 1];
    order[count - 1] = window;
}

struct window *wm_open(const char *title, int x, int y, int client_width, int client_height,
                       const struct window_ops *ops, void *data)
{
    struct window *window = NULL;

    for (size_t i = 0; i < WM_MAX_WINDOWS; i++) {
        if (!pool[i].open) {
            window = &pool[i];
            break;
        }
    }
    if (window == NULL)
        return NULL;

    memset(window, 0, sizeof(*window));
    window->frame.x = x;
    window->frame.y = y < top_limit ? top_limit : y;
    window->frame.w = client_width + 2;
    window->frame.h = client_height + WM_TITLE_HEIGHT + 1;
    strlcpy(window->title, title, sizeof(window->title));
    window->ops = ops;
    window->data = data;
    window->open = true;
    order[count++] = window;
    return window;
}

void wm_close(struct window *window)
{
    size_t index = 0;

    while (index < count && order[index] != window)
        index++;
    if (index == count)
        return;
    if (drag.window == window)
        drag.active = false;
    if (window->ops != NULL && window->ops->closed != NULL)
        window->ops->closed(window);
    for (; index + 1 < count; index++)
        order[index] = order[index + 1];
    order[--count] = NULL;
    window->open = false;
}

void wm_close_all(void)
{
    while (count > 0)
        wm_close(order[count - 1]);
}

struct window *wm_find(const struct window_ops *ops)
{
    for (size_t i = 0; i < count; i++) {
        if (order[i]->ops == ops)
            return order[i];
    }
    return NULL;
}

struct window *wm_close_button_at(int x, int y)
{
    for (size_t i = count; i > 0; i--) {
        struct rect button = wm_close_button_rect(order[i - 1]);

        if (rect_contains(&button, x, y))
            return order[i - 1];
        if (rect_contains(&order[i - 1]->frame, x, y))
            return NULL;
    }
    return NULL;
}

static void draw_close_button(struct gfx *gfx, const struct window *window, bool focused)
{
    struct rect button = wm_close_button_rect(window);
    color_t background = focused ? RGB(196, 64, 64) : RGB(110, 114, 124);
    int left = button.x + 6;
    int top = button.y + 6;
    int size = WM_CLOSE_SIZE - 12;

    gfx_fill_rect(gfx, button, background);
    gfx_line(gfx, left, top, left + size, top + size, RGB(255, 255, 255));
    gfx_line(gfx, left + 1, top, left + size + 1, top + size, RGB(255, 255, 255));
    gfx_line(gfx, left + size, top, left, top + size, RGB(255, 255, 255));
    gfx_line(gfx, left + size + 1, top, left + 1, top + size, RGB(255, 255, 255));
}

static void draw_window(struct gfx *gfx, struct window *window, bool focused)
{
    struct rect shadow = { window->frame.x + SHADOW_OFFSET, window->frame.y + SHADOW_OFFSET, window->frame.w,
                           window->frame.h };
    struct rect title = { window->frame.x, window->frame.y, window->frame.w, WM_TITLE_HEIGHT };
    struct rect client = wm_client_rect(window);
    int text_y = window->frame.y + (WM_TITLE_HEIGHT - GFX_FONT_HEIGHT) / 2;

    gfx_fill_rect_alpha(gfx, shadow, RGB(0, 0, 0), SHADOW_ALPHA);
    gfx_fill_rect(gfx, window->frame, body_color);
    gfx_fill_gradient(gfx, title, focused ? RGB(54, 92, 168) : RGB(82, 86, 98),
                      focused ? RGB(36, 62, 122) : RGB(60, 64, 74));
    gfx_draw_text(gfx, window->frame.x + 12, text_y, window->title, RGB(255, 255, 255));
    draw_close_button(gfx, window, focused);
    gfx_rect_outline(gfx, window->frame, border_color);

    if (window->ops != NULL && window->ops->draw != NULL)
        window->ops->draw(window, gfx, client);
}

void wm_draw(struct gfx *gfx)
{
    for (size_t i = 0; i < count; i++)
        draw_window(gfx, order[i], i + 1 == count);
}

static struct window *window_at(int x, int y)
{
    for (size_t i = count; i > 0; i--) {
        if (rect_contains(&order[i - 1]->frame, x, y))
            return order[i - 1];
    }
    return NULL;
}

static void move_window(struct window *window, int x, int y)
{
    int new_x = x - drag.offset_x;
    int new_y = y - drag.offset_y;
    int max_x = screen_width - MIN_VISIBLE_WIDTH;
    int min_x = MIN_VISIBLE_WIDTH - window->frame.w;
    int max_y = screen_height - WM_TITLE_HEIGHT;

    if (new_x < min_x)
        new_x = min_x;
    if (new_x > max_x)
        new_x = max_x;
    if (new_y < top_limit)
        new_y = top_limit;
    if (new_y > max_y)
        new_y = max_y;
    window->frame.x = new_x;
    window->frame.y = new_y;
}

bool wm_mouse(int x, int y, bool pressed, bool released, bool left_down)
{
    struct window *window;
    struct rect title;
    struct rect client;
    struct rect button;

    if (drag.active) {
        if (released || !left_down) {
            drag.active = false;
            return false;
        }
        move_window(drag.window, x, y);
        return true;
    }
    if (!pressed)
        return false;

    window = window_at(x, y);
    if (window == NULL)
        return false;
    wm_focus(window);

    button = wm_close_button_rect(window);
    if (rect_contains(&button, x, y)) {
        wm_close(window);
        return true;
    }

    title = (struct rect){ window->frame.x, window->frame.y, window->frame.w, WM_TITLE_HEIGHT };
    if (rect_contains(&title, x, y)) {
        drag.active = true;
        drag.window = window;
        drag.offset_x = x - window->frame.x;
        drag.offset_y = y - window->frame.y;
        return true;
    }

    client = wm_client_rect(window);
    if (rect_contains(&client, x, y) && window->ops != NULL && window->ops->click != NULL)
        window->ops->click(window, x - client.x, y - client.y);
    return true;
}

void wm_key(int key)
{
    struct window *window = wm_focused();

    if (window != NULL && window->ops != NULL && window->ops->key != NULL)
        window->ops->key(window, key);
}
