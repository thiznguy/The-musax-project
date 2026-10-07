#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <musax/arch/arch.h>
#include <musax/arch/cpu.h>
#include <musax/console.h>
#include <musax/crash.h>
#include <musax/crash_art.h>
#include <musax/screen.h>

#define ATTRIBUTE_BACKGROUND CONSOLE_ATTRIBUTE(CONSOLE_COLOR_LIGHT_CYAN, CONSOLE_COLOR_BLUE)
#define ATTRIBUTE_ART CONSOLE_ATTRIBUTE(CONSOLE_COLOR_LIGHT_CYAN, CONSOLE_COLOR_BLUE)
#define ATTRIBUTE_ART_HEAVY CONSOLE_ATTRIBUTE(CONSOLE_COLOR_WHITE, CONSOLE_COLOR_BLUE)
#define ATTRIBUTE_TITLE CONSOLE_ATTRIBUTE(CONSOLE_COLOR_WHITE, CONSOLE_COLOR_BLUE)
#define ATTRIBUTE_LABEL CONSOLE_ATTRIBUTE(CONSOLE_COLOR_LIGHT_GREY, CONSOLE_COLOR_BLUE)
#define ATTRIBUTE_CODE CONSOLE_ATTRIBUTE(CONSOLE_COLOR_WHITE, CONSOLE_COLOR_RED)
#define ATTRIBUTE_NO_CODE CONSOLE_ATTRIBUTE(CONSOLE_COLOR_BLACK, CONSOLE_COLOR_LIGHT_GREY)
#define ATTRIBUTE_CAUSE CONSOLE_ATTRIBUTE(CONSOLE_COLOR_YELLOW, CONSOLE_COLOR_BLUE)
#define ATTRIBUTE_DETAIL CONSOLE_ATTRIBUTE(CONSOLE_COLOR_LIGHT_GREY, CONSOLE_COLOR_BLUE)
#define ATTRIBUTE_HINT CONSOLE_ATTRIBUTE(CONSOLE_COLOR_WHITE, CONSOLE_COLOR_BLUE)

#define HINT_TEXT "press any key to restart"

extern char __drivers_text_start[];
extern char __drivers_text_end[];

uint64_t crash_driver_text_start(void)
{
    return (uint64_t)(uintptr_t)__drivers_text_start;
}

uint64_t crash_driver_text_end(void)
{
    return (uint64_t)(uintptr_t)__drivers_text_end;
}

static size_t centered(size_t width)
{
    return width < CONSOLE_COLUMNS ? (CONSOLE_COLUMNS - width) / 2 : 0;
}

static void draw_centered(size_t row, const char *text, uint8_t attribute)
{
    size_t length = strlen(text);

    if (length > CONSOLE_COLUMNS)
        length = CONSOLE_COLUMNS;
    screen_write(row, centered(length), text, length, attribute);
}

static const char *cause_text(enum crash_class class)
{
    switch (class) {
    case CRASH_DRIVER:
        return "A driver caused this crash.";
    case CRASH_HARDWARE:
        return "Your CPU or motherboard may be faulty.";
    default:
        return "The cause of this crash could not be identified.";
    }
}

static void draw_art(const struct crash_art *art, size_t top)
{
    size_t left = centered(art->width);

    for (size_t row = 0; row < art->row_count; row++) {
        const char *line = art->rows[row];

        for (size_t column = 0; column < art->width; column++) {
            char c = line[column];

            if (c == ' ')
                continue;
            screen_put(top + row, left + column, (uint8_t)c,
                       (c == '#' || c == '*') ? ATTRIBUTE_ART_HEAVY : ATTRIBUTE_ART);
        }
    }
}

static void draw_code(size_t row, enum crash_class class)
{
    char text[40];
    const char *code = crash_class_code(class);
    size_t label_length;
    size_t badge_length;
    size_t total;
    size_t column;

    if (code != NULL) {
        snprintf(text, sizeof(text), " %s ", code);
    } else {
        snprintf(text, sizeof(text), " not identified yet ");
    }
    label_length = strlen("error code: ");
    badge_length = strlen(text);
    total = label_length + badge_length;
    column = centered(total);

    screen_write(row, column, "error code: ", label_length, ATTRIBUTE_LABEL);
    screen_write(row, column + label_length, text, badge_length, code != NULL ? ATTRIBUTE_CODE : ATTRIBUTE_NO_CODE);
}

void crash_draw(const struct crash_report *report, size_t rows)
{
    const struct crash_art *art = rows >= CRASH_FULL_LAYOUT_ROWS ? &crash_art_full : &crash_art_compact;
    int spacious = rows >= CRASH_FULL_LAYOUT_ROWS;
    size_t content = art->row_count + (spacious ? 10 : 7);
    size_t top = rows > content ? (rows - content) / 2 : 0;
    size_t row;

    screen_clear(ATTRIBUTE_BACKGROUND);
    draw_art(art, top);

    row = top + art->row_count + 1;
    draw_centered(row++, CRASH_TITLE, ATTRIBUTE_TITLE);
    if (spacious)
        row++;
    draw_code(row++, report->class);
    draw_centered(row++, cause_text(report->class), ATTRIBUTE_CAUSE);
    if (report->summary != NULL)
        draw_centered(row++, report->summary, ATTRIBUTE_DETAIL);
    if (report->detail != NULL)
        draw_centered(row++, report->detail, ATTRIBUTE_DETAIL);
    if (spacious)
        row++;
    draw_centered(row, HINT_TEXT, ATTRIBUTE_HINT);
}

void crash_show(const struct crash_report *report)
{
    static volatile int showing;

    cpu_disable_interrupts();
    if (showing)
        cpu_halt_forever();
    showing = 1;

    console_prepare_for_crash();
    screen_begin();
    crash_draw(report, console_rows());
    screen_flush_hidden();

    while (console_poll_key() < 0)
        cpu_relax();

    arch_reboot();
}
