#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <musax/boot.h>
#include <musax/console.h>
#include <musax/drivers/timer.h>
#include <musax/kprintf.h>
#include <musax/panic.h>
#include <musax/screen.h>
#include <musax/version.h>

#define LAYOUT_ROWS 25
#define TOP_BAR_ROW 0
#define LOGO_ROW 2
#define LOGO_HEIGHT 5
#define TAGLINE_ROW 8
#define PANEL_ROW 10
#define PANEL_LEFT 10
#define PANEL_WIDTH 60
#define PANEL_LINES 11
#define PROGRESS_ROW 23
#define HINT_ROW 24
#define PROGRESS_CELLS 50
#define STAR_COUNT 70
#define STAGE_MIN_FRAMES 6
#define HOLD_FRAMES 14
#define LOGO_REVEAL_STEP 3
#define SHINE_PERIOD 170
#define MILLI 1000U
#define PIXEL_COLUMNS 2
#define LOGO_GAP_PIXELS 1

#define GLYPH_FULL_BLOCK 0xDB
#define GLYPH_LOWER_HALF 0xDC
#define GLYPH_UPPER_HALF 0xDF
#define GLYPH_SHADE_LIGHT 0xB0
#define GLYPH_SHADE_MEDIUM 0xB1
#define GLYPH_CHECK 0xFB
#define GLYPH_DOT 0xFA
#define GLYPH_BOX_TOP_LEFT 0xC9
#define GLYPH_BOX_TOP_RIGHT 0xBB
#define GLYPH_BOX_BOTTOM_LEFT 0xC8
#define GLYPH_BOX_BOTTOM_RIGHT 0xBC
#define GLYPH_BOX_HORIZONTAL 0xCD
#define GLYPH_BOX_VERTICAL 0xBA

#define ATTR(foreground, background) CONSOLE_ATTRIBUTE(foreground, background)
#define BLACK CONSOLE_COLOR_BLACK

enum stage_state {
    STAGE_PENDING,
    STAGE_ACTIVE,
    STAGE_DONE,
    STAGE_FAILED,
};

struct stage_view {
    enum stage_state state;
    char detail[BOOT_DETAIL_MAX + 1];
};

struct star {
    uint8_t row;
    uint8_t column;
    uint8_t phase;
    uint8_t speed;
};

struct splash {
    const struct boot_stage *stages;
    size_t count;
    struct stage_view views[BOOT_MAX_STAGES];
    size_t current;
    unsigned frame_delay_ms;
    uint32_t frame;
    int reveal;
    uint32_t progress_milli;
    uint32_t target_milli;
    size_t top;
    size_t rows;
    bool skipped;
    struct star stars[STAR_COUNT];
};

/* A 5x5 pixel letter: width in pixels and one bit mask per row, most significant bit leftmost. */
struct glyph {
    uint8_t width;
    uint8_t rows[LOGO_HEIGHT];
};

static const struct glyph glyph_m = { 5, { 0x11, 0x1B, 0x15, 0x11, 0x11 } };
static const struct glyph glyph_u = { 5, { 0x11, 0x11, 0x11, 0x11, 0x0E } };
static const struct glyph glyph_s = { 5, { 0x0F, 0x10, 0x0E, 0x01, 0x1E } };
static const struct glyph glyph_a = { 5, { 0x0E, 0x11, 0x1F, 0x11, 0x11 } };
static const struct glyph glyph_x = { 5, { 0x11, 0x0A, 0x04, 0x0A, 0x11 } };
static const struct glyph glyph_dash = { 3, { 0x00, 0x00, 0x07, 0x00, 0x00 } };
static const struct glyph glyph_one = { 3, { 0x02, 0x06, 0x02, 0x02, 0x07 } };

static const struct glyph *const logo_glyphs[] = {
    &glyph_m, &glyph_u, &glyph_s, &glyph_a, &glyph_x, &glyph_dash, &glyph_one,
};

static const uint8_t logo_row_colors[LOGO_HEIGHT] = {
    CONSOLE_COLOR_WHITE, CONSOLE_COLOR_LIGHT_CYAN, CONSOLE_COLOR_LIGHT_CYAN, CONSOLE_COLOR_CYAN,
    CONSOLE_COLOR_LIGHT_BLUE,
};

static const uint8_t wave_colors[] = {
    CONSOLE_COLOR_BLUE,       CONSOLE_COLOR_BLUE, CONSOLE_COLOR_LIGHT_BLUE, CONSOLE_COLOR_CYAN,
    CONSOLE_COLOR_LIGHT_CYAN, CONSOLE_COLOR_CYAN, CONSOLE_COLOR_LIGHT_BLUE, CONSOLE_COLOR_BLUE,
};

static const uint8_t gradient_colors[] = {
    CONSOLE_COLOR_BLUE, CONSOLE_COLOR_LIGHT_BLUE, CONSOLE_COLOR_CYAN, CONSOLE_COLOR_LIGHT_CYAN, CONSOLE_COLOR_WHITE,
};

static const char spinner_frames[] = "|/-\\";

static size_t logo_pixel_width(void)
{
    size_t width = 0;

    for (size_t i = 0; i < sizeof(logo_glyphs) / sizeof(logo_glyphs[0]); i++)
        width += logo_glyphs[i]->width + (i > 0 ? LOGO_GAP_PIXELS : 0);
    return width;
}

static size_t logo_columns(void)
{
    return logo_pixel_width() * PIXEL_COLUMNS;
}

static size_t centered(size_t width)
{
    return width < CONSOLE_COLUMNS ? (CONSOLE_COLUMNS - width) / 2 : 0;
}

static uint32_t next_random(uint32_t *state)
{
    *state = *state * 1664525U + 1013904223U;
    return *state >> 8;
}

static void init_stars(struct splash *s)
{
    uint32_t state = 0x6D757361U;

    for (size_t i = 0; i < STAR_COUNT; i++) {
        s->stars[i].row = (uint8_t)(1 + next_random(&state) % (s->rows - 1));
        s->stars[i].column = (uint8_t)(next_random(&state) % CONSOLE_COLUMNS);
        s->stars[i].phase = (uint8_t)(next_random(&state) % 16);
        s->stars[i].speed = (uint8_t)(3 + next_random(&state) % 5);
    }
}

static void draw_stars(const struct splash *s)
{
    for (size_t i = 0; i < STAR_COUNT; i++) {
        const struct star *star = &s->stars[i];
        unsigned level = (s->frame / star->speed + star->phase) % 8;

        if (level == 0 || level > 4)
            continue;
        if (level == 1 || level == 4)
            screen_put(star->row, star->column, '.', ATTR(CONSOLE_COLOR_DARK_GREY, BLACK));
        else if (level == 2)
            screen_put(star->row, star->column, GLYPH_DOT, ATTR(CONSOLE_COLOR_LIGHT_GREY, BLACK));
        else
            screen_put(star->row, star->column, '+', ATTR(CONSOLE_COLOR_WHITE, BLACK));
    }
}

static void draw_bar(const struct splash *s, size_t row)
{
    for (size_t column = 0; column < CONSOLE_COLUMNS; column++) {
        size_t index = (column + s->frame / 2) % (sizeof(wave_colors) / sizeof(wave_colors[0]));

        screen_put(row, column, GLYPH_LOWER_HALF, ATTR(wave_colors[index], BLACK));
    }
}

static void draw_logo(const struct splash *s)
{
    size_t start = centered(logo_columns() + PIXEL_COLUMNS);
    size_t shine_position = (s->frame * 3) % SHINE_PERIOD;
    bool revealed = (size_t)s->reveal >= logo_columns() + PIXEL_COLUMNS;

    for (int pass = 0; pass < 2; pass++) {
        size_t pixel_x = 0;

        for (size_t g = 0; g < sizeof(logo_glyphs) / sizeof(logo_glyphs[0]); g++) {
            const struct glyph *glyph = logo_glyphs[g];

            for (size_t gy = 0; gy < LOGO_HEIGHT; gy++) {
                for (size_t gx = 0; gx < glyph->width; gx++) {
                    size_t cell_x;
                    size_t row;
                    size_t column;

                    if (!(glyph->rows[gy] & (1U << (glyph->width - 1 - gx))))
                        continue;
                    cell_x = (pixel_x + gx) * PIXEL_COLUMNS;
                    if ((int)cell_x >= s->reveal)
                        continue;

                    row = s->top + LOGO_ROW + gy + (pass == 0 ? 1 : 0);
                    column = start + cell_x + (pass == 0 ? PIXEL_COLUMNS : 0);
                    for (size_t part = 0; part < PIXEL_COLUMNS; part++) {
                        if (pass == 0) {
                            screen_put(row, column + part, GLYPH_SHADE_MEDIUM, ATTR(CONSOLE_COLOR_BLUE, BLACK));
                        } else {
                            uint8_t color = logo_row_colors[gy];
                            size_t diagonal = (cell_x + part) + 3 * gy;
                            bool edge = !revealed && (int)cell_x >= s->reveal - 4;
                            bool shine = revealed && diagonal + 2 >= shine_position && diagonal <= shine_position + 2;

                            if (edge || shine)
                                color = CONSOLE_COLOR_WHITE;
                            screen_put(row, column + part, GLYPH_FULL_BLOCK, ATTR(color, BLACK));
                        }
                    }
                }
            }
            pixel_x += glyph->width + LOGO_GAP_PIXELS;
        }
    }
}

static size_t draw_segment(size_t row, size_t column, const char *text, uint8_t attribute)
{
    size_t length = strlen(text);

    screen_write(row, column, text, length, attribute);
    return column + length;
}

static void draw_tagline(const struct splash *s)
{
    static const char separator[] = "  \xFA  ";
    char version[24];
    size_t row = s->top + TAGLINE_ROW;
    size_t total;
    size_t column;

    snprintf(version, sizeof(version), "version %s", MUSAX_VERSION);
    total = strlen("portable kernel") + strlen(separator) + strlen("muSIC shell") + strlen(separator) + strlen(version);
    column = centered(total);

    column = draw_segment(row, column, "portable kernel", ATTR(CONSOLE_COLOR_LIGHT_GREY, BLACK));
    column = draw_segment(row, column, separator, ATTR(CONSOLE_COLOR_DARK_GREY, BLACK));
    column = draw_segment(row, column, "muSIC shell", ATTR(CONSOLE_COLOR_LIGHT_CYAN, BLACK));
    column = draw_segment(row, column, separator, ATTR(CONSOLE_COLOR_DARK_GREY, BLACK));
    draw_segment(row, column, version, ATTR(CONSOLE_COLOR_WHITE, BLACK));
}

static void draw_panel_frame(const struct splash *s)
{
    size_t top = s->top + PANEL_ROW;
    size_t bottom = top + PANEL_LINES + 1;
    uint8_t border = ATTR(CONSOLE_COLOR_LIGHT_BLUE, BLACK);
    size_t title_column = PANEL_LEFT + 3;

    for (size_t row = top; row <= bottom; row++) {
        for (size_t column = PANEL_LEFT; column < PANEL_LEFT + PANEL_WIDTH; column++) {
            uint8_t glyph = ' ';
            bool left = column == PANEL_LEFT;
            bool right = column == PANEL_LEFT + PANEL_WIDTH - 1;

            if (row == top)
                glyph = left ? GLYPH_BOX_TOP_LEFT : right ? GLYPH_BOX_TOP_RIGHT : GLYPH_BOX_HORIZONTAL;
            else if (row == bottom)
                glyph = left ? GLYPH_BOX_BOTTOM_LEFT : right ? GLYPH_BOX_BOTTOM_RIGHT : GLYPH_BOX_HORIZONTAL;
            else if (left || right)
                glyph = GLYPH_BOX_VERTICAL;
            screen_put(row, column, glyph, border);
        }
    }
    draw_segment(top, title_column, " system startup ", ATTR(CONSOLE_COLOR_WHITE, BLACK));
}

static void draw_stage_line(const struct splash *s, size_t index, size_t line)
{
    const struct stage_view *view = &s->views[index];
    size_t row = s->top + PANEL_ROW + 1 + line;
    size_t glyph_column = PANEL_LEFT + 2;
    size_t label_column = glyph_column + 2;
    size_t right_edge = PANEL_LEFT + PANEL_WIDTH - 3;
    size_t label_length = strlen(s->stages[index].label);
    size_t detail_length = strlen(view->detail);
    uint8_t label_color = CONSOLE_COLOR_LIGHT_GREY;
    uint8_t glyph_color = CONSOLE_COLOR_DARK_GREY;
    uint8_t glyph = GLYPH_DOT;

    switch (view->state) {
    case STAGE_ACTIVE:
        glyph = (uint8_t)spinner_frames[(s->frame / 2) % 4];
        glyph_color = CONSOLE_COLOR_YELLOW;
        label_color = CONSOLE_COLOR_WHITE;
        break;
    case STAGE_DONE:
        glyph = GLYPH_CHECK;
        glyph_color = CONSOLE_COLOR_LIGHT_GREEN;
        break;
    case STAGE_FAILED:
        glyph = 'x';
        glyph_color = CONSOLE_COLOR_LIGHT_RED;
        label_color = CONSOLE_COLOR_LIGHT_RED;
        break;
    default:
        label_color = CONSOLE_COLOR_DARK_GREY;
        break;
    }

    screen_put(row, glyph_column, glyph, ATTR(glyph_color, BLACK));
    screen_write(row, label_column, s->stages[index].label, label_length, ATTR(label_color, BLACK));

    if (detail_length > 0) {
        size_t detail_column = right_edge + 1 - detail_length;

        for (size_t column = label_column + label_length + 1; column + 1 < detail_column; column++)
            screen_put(row, column, '.', ATTR(CONSOLE_COLOR_DARK_GREY, BLACK));
        screen_write(row, detail_column, view->detail, detail_length,
                     ATTR(view->state == STAGE_FAILED ? CONSOLE_COLOR_LIGHT_RED : CONSOLE_COLOR_LIGHT_CYAN, BLACK));
    }
}

static void draw_stage_list(const struct splash *s)
{
    size_t first = s->current >= PANEL_LINES ? s->current + 1 - PANEL_LINES : 0;

    for (size_t line = 0; line < PANEL_LINES && first + line < s->count; line++)
        draw_stage_line(s, first + line, line);
}

static void draw_progress(const struct splash *s)
{
    size_t row = s->top + PROGRESS_ROW;
    size_t total = 1 + PROGRESS_CELLS + 1 + 1 + 4;
    size_t column = centered(total);
    size_t filled = (size_t)((uint64_t)s->progress_milli * PROGRESS_CELLS / MILLI);
    char percent[8];

    screen_put(row, column, '[', ATTR(CONSOLE_COLOR_LIGHT_GREY, BLACK));
    for (size_t i = 0; i < PROGRESS_CELLS; i++) {
        if (i < filled) {
            size_t shade = i * (sizeof(gradient_colors) / sizeof(gradient_colors[0])) / PROGRESS_CELLS;

            screen_put(row, column + 1 + i, GLYPH_FULL_BLOCK, ATTR(gradient_colors[shade], BLACK));
        } else {
            screen_put(row, column + 1 + i, GLYPH_SHADE_LIGHT, ATTR(CONSOLE_COLOR_DARK_GREY, BLACK));
        }
    }
    screen_put(row, column + 1 + PROGRESS_CELLS, ']', ATTR(CONSOLE_COLOR_LIGHT_GREY, BLACK));
    snprintf(percent, sizeof(percent), "%3u%%", (unsigned)(s->progress_milli / 10));
    screen_write(row, column + PROGRESS_CELLS + 3, percent, strlen(percent), ATTR(CONSOLE_COLOR_WHITE, BLACK));
}

static void draw_hint(const struct splash *s, const char *text)
{
    size_t length = strlen(text);

    screen_write(s->top + HINT_ROW, centered(length), text, length, ATTR(CONSOLE_COLOR_DARK_GREY, BLACK));
}

static void render(const struct splash *s, const char *hint)
{
    screen_clear(ATTR(CONSOLE_COLOR_LIGHT_GREY, BLACK));
    draw_stars(s);
    draw_bar(s, TOP_BAR_ROW);
    if (s->rows > LAYOUT_ROWS)
        draw_bar(s, s->rows - 1);
    draw_logo(s);
    if (s->reveal > 0)
        draw_tagline(s);
    draw_panel_frame(s);
    if (s->count > 0)
        draw_stage_list(s);
    draw_progress(s);
    draw_hint(s, hint);
    screen_flush_hidden();
}

static void advance_frame(struct splash *s, const char *hint)
{
    uint32_t distance;

    s->frame++;
    if ((size_t)s->reveal < logo_columns() + PIXEL_COLUMNS)
        s->reveal += LOGO_REVEAL_STEP;

    if (s->target_milli > s->progress_milli) {
        distance = (s->target_milli - s->progress_milli) / 4 + 1;
        s->progress_milli = s->progress_milli + distance > s->target_milli ? s->target_milli
                                                                            : s->progress_milli + distance;
    }

    render(s, hint);
    timer_delay_ms(s->frame_delay_ms);
    if (console_poll_key() >= 0)
        s->skipped = true;
}

static void init_splash(struct splash *s, const struct boot_stage *stages, size_t count, unsigned frame_delay_ms)
{
    memset(s, 0, sizeof(*s));
    s->stages = stages;
    s->count = count;
    s->frame_delay_ms = frame_delay_ms;
    s->rows = console_rows();
    s->top = s->rows > LAYOUT_ROWS ? (s->rows - LAYOUT_ROWS) / 2 : 0;
    init_stars(s);
}

static void wipe_out(struct splash *s)
{
    for (size_t row = 0; row < s->rows; row++) {
        for (size_t above = 0; above < row; above++)
            screen_fill_row(above, ' ', ATTR(CONSOLE_COLOR_LIGHT_GREY, BLACK));
        screen_fill_row(row, GLYPH_UPPER_HALF, ATTR(CONSOLE_COLOR_WHITE, BLACK));
        if (row + 1 < s->rows)
            screen_fill_row(row + 1, GLYPH_LOWER_HALF, ATTR(CONSOLE_COLOR_LIGHT_CYAN, BLACK));
        screen_flush_hidden();
        timer_delay_ms(s->frame_delay_ms / 3 + 1);
    }
}

static void log_stage(const struct boot_stage *stage, int status, const char *detail)
{
    kprintf("[ %s ] %s%s%s\n", status == 0 ? " ok " : "FAIL", stage->label, detail[0] != '\0' ? ": " : "", detail);
}

static void run_stage_logged(const struct boot_stage *stage)
{
    char detail[BOOT_DETAIL_MAX + 1] = "";
    int status = stage->run(detail, sizeof(detail));

    log_stage(stage, status, detail);
    if (status != 0)
        kpanic("boot stage failed: %s: %s", stage->label, detail);
}

void boot_run(const struct boot_stage *stages, size_t count, bool animated, unsigned frame_delay_ms)
{
    static struct splash splash;
    size_t shown = count < BOOT_MAX_STAGES ? count : BOOT_MAX_STAGES;

    if (!animated) {
        for (size_t i = 0; i < count; i++)
            run_stage_logged(&stages[i]);
        return;
    }

    init_splash(&splash, stages, shown, frame_delay_ms);
    screen_begin();

    while (!splash.skipped && (size_t)splash.reveal < logo_columns() + PIXEL_COLUMNS)
        advance_frame(&splash, "press any key to skip");

    for (size_t i = 0; i < count; i++) {
        struct stage_view *view = &splash.views[i < shown ? i : shown - 1];
        int status;

        splash.current = i < shown ? i : shown - 1;
        view->state = STAGE_ACTIVE;
        view->detail[0] = '\0';
        splash.target_milli = (uint32_t)(MILLI * i / count);
        if (!splash.skipped)
            advance_frame(&splash, "press any key to skip");

        status = stages[i].run(view->detail, sizeof(view->detail));
        view->state = status == 0 ? STAGE_DONE : STAGE_FAILED;
        splash.target_milli = (uint32_t)(MILLI * (i + 1) / count);

        if (status != 0) {
            if (!splash.skipped)
                advance_frame(&splash, "startup failed");
            kpanic("boot stage failed: %s: %s", stages[i].label, view->detail);
        }

        for (unsigned frame = 0; frame < STAGE_MIN_FRAMES && !splash.skipped; frame++)
            advance_frame(&splash, "press any key to skip");
    }

    if (splash.skipped) {
        screen_end();
        for (size_t i = 0; i < count; i++)
            log_stage(&stages[i], 0, splash.views[i < shown ? i : shown - 1].detail);
        return;
    }

    splash.target_milli = MILLI;
    for (unsigned frame = 0; frame < HOLD_FRAMES && !splash.skipped; frame++)
        advance_frame(&splash, "starting muSIC");
    if (!splash.skipped)
        wipe_out(&splash);
    screen_end();
}

void boot_splash_demo(unsigned frame_delay_ms)
{
    static struct splash splash;
    static const char *const labels[] = { "Detecting CPU", "Installing interrupts", "Starting memory manager" };

    (void)labels;
    init_splash(&splash, NULL, 0, frame_delay_ms);
    screen_begin();
    while (!splash.skipped && (size_t)splash.reveal < logo_columns() + PIXEL_COLUMNS)
        advance_frame(&splash, "press any key to continue");
    splash.target_milli = MILLI;
    for (unsigned frame = 0; frame < 60 && !splash.skipped; frame++)
        advance_frame(&splash, "press any key to continue");
    if (!splash.skipped)
        wipe_out(&splash);
    screen_end();
}
