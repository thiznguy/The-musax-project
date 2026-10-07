#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <musax/boot/bootinfo.h>
#include <musax/clock.h>
#include <musax/config.h>
#include <musax/console.h>
#include <musax/drivers/display.h>
#include <musax/drivers/ps2_mouse.h>
#include <musax/drivers/timer.h>
#include <musax/drivers/vga_text.h>
#include <musax/gui/apps.h>
#include <musax/gui/desktop.h>
#include <musax/gui/gfx.h>
#include <musax/gui/theme.h>
#include <musax/gui/wm.h>
#include <musax/keys.h>

#define BUTTON_PADDING 12
#define BUTTON_HEIGHT 20
#define BUTTON_TOP 4
#define LOGO_X 14
#define CLOCK_MARGIN 14
#define CLOCK_REFRESH_MS 1000
#define HINT_LINES 3

struct button {
    const char *label;
    struct rect rect;
    void (*action)(void);
};

static struct display_mode mode;
static struct gfx screen;
static struct gfx scene;
static uint32_t *scene_pixels;

static int mouse_x;
static int mouse_y;
static bool mouse_left;
static bool mouse_present;
static bool quit_requested;
static bool redraw_needed;
static size_t current_theme;
static char clock_text[8];
static uint64_t last_clock_ms;
static int hovered_button = -1;
static const struct window *hovered_close;

static void action_terminal(void);
static void action_settings(void);
static void action_exit(void);

static struct button buttons[] = {
    { "Terminal", { 0, 0, 0, 0 }, action_terminal },
    { "Settings", { 0, 0, 0, 0 }, action_settings },
    { "Exit", { 0, 0, 0, 0 }, action_exit },
};

#define BUTTON_COUNT (sizeof(buttons) / sizeof(buttons[0]))

static void action_terminal(void)
{
    terminal_open();
}

static void action_settings(void)
{
    settings_open();
}

static void action_exit(void)
{
    quit_requested = true;
}

const char *desktop_error_message(int error)
{
    switch (error) {
    case DESKTOP_ERR_NO_FONT:
        return "the boot loader did not provide a screen font";
    case DESKTOP_ERR_NO_DISPLAY:
        return "no Bochs/QEMU display adapter found (start QEMU with -vga std)";
    case DESKTOP_ERR_NO_MEMORY:
        return "not enough memory for the screen buffer";
    default:
        return "unknown error";
    }
}

void desktop_request_redraw(void)
{
    redraw_needed = true;
}

size_t desktop_theme(void)
{
    return current_theme;
}

void desktop_set_theme(size_t index)
{
    current_theme = index < theme_count() ? index : 0;
    config_set(DESKTOP_THEME_KEY, theme_at(current_theme)->name);
    config_save();
    redraw_needed = true;
}

static void layout_buttons(void)
{
    int x = LOGO_X + gfx_text_width("musax-1") + 20;
    size_t last = BUTTON_COUNT - 1;

    for (size_t i = 0; i < last; i++) {
        int width = gfx_text_width(buttons[i].label) + 2 * BUTTON_PADDING;

        buttons[i].rect = (struct rect){ x, BUTTON_TOP, width, BUTTON_HEIGHT };
        x += width + 6;
    }
    {
        int width = gfx_text_width(buttons[last].label) + 2 * BUTTON_PADDING;

        buttons[last].rect = (struct rect){ DESKTOP_WIDTH - width - 8, BUTTON_TOP, width, BUTTON_HEIGHT };
    }
}

static void update_clock(void)
{
    time_t now;
    struct tm date;

    if (!clock_now(&now)) {
        strcpy(clock_text, "--:--");
        return;
    }
    gmtime_r(&now, &date);
    snprintf(clock_text, sizeof(clock_text), "%02d:%02d", date.tm_hour, date.tm_min);
}

static void draw_top_bar(void)
{
    struct rect bar = { 0, 0, DESKTOP_WIDTH, DESKTOP_TOP_BAR_HEIGHT };
    struct rect line = { 0, DESKTOP_TOP_BAR_HEIGHT - 1, DESKTOP_WIDTH, 1 };
    int text_y = (DESKTOP_TOP_BAR_HEIGHT - 1 - GFX_FONT_HEIGHT) / 2;

    gfx_fill_gradient(&scene, bar, RGB(24, 28, 44), RGB(14, 17, 28));
    gfx_fill_rect(&scene, line, RGB(58, 96, 168));
    gfx_draw_text(&scene, LOGO_X, text_y, "musax-1", RGB(120, 220, 255));

    for (size_t i = 0; i < BUTTON_COUNT; i++) {
        const struct button *button = &buttons[i];
        bool hovered = (int)i == hovered_button;
        int label_x = button->rect.x + (button->rect.w - gfx_text_width(button->label)) / 2;
        int label_y = button->rect.y + (button->rect.h - GFX_FONT_HEIGHT) / 2;

        gfx_fill_rect(&scene, button->rect, hovered ? RGB(70, 112, 196) : RGB(40, 48, 72));
        gfx_draw_text(&scene, label_x, label_y, button->label, RGB(255, 255, 255));
    }

    gfx_draw_text(&scene, buttons[BUTTON_COUNT - 1].rect.x - CLOCK_MARGIN - gfx_text_width(clock_text), text_y,
                  clock_text, RGB(220, 226, 240));
}

static void draw_hint(void)
{
    static const char *const lines[HINT_LINES] = {
        "musax-1 desktop",
        "Alt+G  terminal      Alt+S  settings      Alt+Q  exit",
        "Move the mouse over the top bar, or use the keys above.",
    };
    static const color_t colors[HINT_LINES] = { RGB(255, 255, 255), RGB(220, 232, 255), RGB(170, 190, 220) };
    int top = DESKTOP_HEIGHT / 2 - (HINT_LINES * 24) / 2;

    for (int i = 0; i < HINT_LINES; i++) {
        int x = (DESKTOP_WIDTH - gfx_text_width(lines[i])) / 2;

        gfx_draw_text(&scene, x, top + i * 24, lines[i], colors[i]);
    }
}

static void render_scene(void)
{
    const struct theme *theme = theme_at(current_theme);
    struct rect full = { 0, 0, DESKTOP_WIDTH, DESKTOP_HEIGHT };

    gfx_fill_gradient(&scene, full, theme->top, theme->bottom);
    if (wm_window_count() == 0)
        draw_hint();
    wm_draw(&scene);
    draw_top_bar();
}

static struct rect cursor_rect(void)
{
    struct rect rect = { mouse_x, mouse_y, CURSOR_WIDTH, CURSOR_HEIGHT };

    return rect;
}

static void present(struct rect rect)
{
    struct rect cursor = cursor_rect();

    gfx_copy_rect(&screen, &scene, rect);
    if (mouse_present && rect_intersects(&rect, &cursor))
        gfx_draw_cursor(&screen, mouse_x, mouse_y);
}

static void present_all(void)
{
    struct rect full = { 0, 0, DESKTOP_WIDTH, DESKTOP_HEIGHT };

    present(full);
}

static int button_at(int x, int y)
{
    for (size_t i = 0; i < BUTTON_COUNT; i++) {
        if (rect_contains(&buttons[i].rect, x, y))
            return (int)i;
    }
    return -1;
}

static void handle_key(int key)
{
    struct window *focused;

    switch (key) {
    case KEY_ALT('g'):
        terminal_open();
        break;
    case KEY_ALT('s'):
        settings_open();
        break;
    case KEY_ALT('q'):
        quit_requested = true;
        break;
    case KEY_ALT('w'):
        focused = wm_focused();
        if (focused != NULL)
            wm_close(focused);
        break;
    default:
        wm_key(key);
        break;
    }
    redraw_needed = true;
}

static void handle_mouse_event(const struct mouse_event *event)
{
    struct rect before = cursor_rect();
    bool left = (event->buttons & MOUSE_BUTTON_LEFT) != 0;
    bool pressed = left && !mouse_left;
    bool released = !left && mouse_left;
    int button;
    const struct window *close_hover;

    mouse_x += event->dx;
    mouse_y += event->dy;
    if (mouse_x < 0)
        mouse_x = 0;
    if (mouse_y < 0)
        mouse_y = 0;
    if (mouse_x >= DESKTOP_WIDTH)
        mouse_x = DESKTOP_WIDTH - 1;
    if (mouse_y >= DESKTOP_HEIGHT)
        mouse_y = DESKTOP_HEIGHT - 1;
    mouse_left = left;

    button = button_at(mouse_x, mouse_y);
    close_hover = wm_close_button_at(mouse_x, mouse_y);
    if (button != hovered_button || close_hover != hovered_close) {
        hovered_button = button;
        hovered_close = close_hover;
        redraw_needed = true;
    }

    if (pressed && button >= 0) {
        buttons[button].action();
        redraw_needed = true;
    } else if (wm_mouse(mouse_x, mouse_y, pressed, released, left)) {
        redraw_needed = true;
    }

    if (!redraw_needed) {
        struct rect after = cursor_rect();

        present(before);
        present(after);
    }
}

static void emergency_exit(void)
{
    display_leave_graphics();
    console_set_graphical(false);
}

int desktop_run(void)
{
    const uint8_t *font = bootinfo_font();
    struct mouse_event event;

    if (font == NULL || !gfx_set_font(font))
        return DESKTOP_ERR_NO_FONT;
    if (!display_enter_graphics(DESKTOP_WIDTH, DESKTOP_HEIGHT, &mode))
        return DESKTOP_ERR_NO_DISPLAY;

    scene_pixels = malloc((size_t)DESKTOP_WIDTH * DESKTOP_HEIGHT * sizeof(uint32_t));
    if (scene_pixels == NULL) {
        display_leave_graphics();
        return DESKTOP_ERR_NO_MEMORY;
    }

    screen = (struct gfx){ mode.framebuffer, mode.width, mode.height, mode.pitch_pixels };
    scene = (struct gfx){ scene_pixels, DESKTOP_WIDTH, DESKTOP_HEIGHT, DESKTOP_WIDTH };

    console_set_graphical(true);
    console_set_crash_hook(emergency_exit);
    terminal_attach_output();

    mouse_present = ps2_mouse_init();
    mouse_x = DESKTOP_WIDTH / 2;
    mouse_y = DESKTOP_HEIGHT / 2;
    mouse_left = false;
    quit_requested = false;
    hovered_button = -1;
    hovered_close = NULL;
    current_theme = theme_index_by_name(config_get(DESKTOP_THEME_KEY, "ocean"));

    wm_init(DESKTOP_WIDTH, DESKTOP_HEIGHT, DESKTOP_TOP_BAR_HEIGHT);
    layout_buttons();
    update_clock();
    last_clock_ms = timer_uptime_ms();

    render_scene();
    present_all();
    redraw_needed = false;

    while (!quit_requested) {
        int key;

        while ((key = console_poll_key()) >= 0)
            handle_key(key);
        while (ps2_mouse_read(&event))
            handle_mouse_event(&event);

        if (timer_uptime_ms() - last_clock_ms >= CLOCK_REFRESH_MS) {
            char previous[sizeof(clock_text)];

            last_clock_ms = timer_uptime_ms();
            strcpy(previous, clock_text);
            update_clock();
            if (strcmp(previous, clock_text) != 0)
                redraw_needed = true;
        }

        if (redraw_needed) {
            redraw_needed = false;
            render_scene();
            present_all();
        }
        console_run_idle_hooks();
        timer_wait_for_tick();
    }

    wm_close_all();
    terminal_detach_output();
    console_set_crash_hook(NULL);
    display_leave_graphics();
    console_set_graphical(false);
    vga_text_clear();
    free(scene_pixels);
    scene_pixels = NULL;
    return DESKTOP_OK;
}
