#ifndef MUSAX_GUI_WM_H
#define MUSAX_GUI_WM_H

#include <stdbool.h>
#include <musax/gui/gfx.h>

#define WM_MAX_WINDOWS 8
#define WM_TITLE_HEIGHT 28
#define WM_TITLE_MAX 32
#define WM_CLOSE_SIZE 20

struct window;

/* Application callbacks. Coordinates passed to click are relative to the client area. */
struct window_ops {
    void (*draw)(struct window *window, struct gfx *gfx, struct rect client);
    void (*click)(struct window *window, int x, int y);
    void (*key)(struct window *window, int key);
    void (*closed)(struct window *window);
};

struct window {
    struct rect frame;
    char title[WM_TITLE_MAX];
    const struct window_ops *ops;
    void *data;
    bool open;
};

/* Sets the screen size and the lowest y a title bar may be dragged to (below the top bar). */
void wm_init(int screen_width, int screen_height, int top_limit);

/* Opens a window with a client area of the given size on top of all others and focuses it. */
struct window *wm_open(const char *title, int x, int y, int client_width, int client_height,
                       const struct window_ops *ops, void *data);
void wm_close(struct window *window);
void wm_close_all(void);
void wm_focus(struct window *window);
struct window *wm_focused(void);
struct window *wm_find(const struct window_ops *ops);
size_t wm_window_count(void);

struct rect wm_client_rect(const struct window *window);
struct rect wm_close_button_rect(const struct window *window);

/* The window whose close button lies under the point, or NULL. */
struct window *wm_close_button_at(int x, int y);

void wm_draw(struct gfx *gfx);

/* Feeds one mouse state. Returns true when the picture changed. */
bool wm_mouse(int x, int y, bool pressed, bool released, bool left_down);
void wm_key(int key);

#endif
