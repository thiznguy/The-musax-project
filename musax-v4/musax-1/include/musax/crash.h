#ifndef MUSAX_CRASH_H
#define MUSAX_CRASH_H

#include <stddef.h>
#include <stdint.h>

/* Crash causes the kernel can tell apart. Each classified cause has its own error code on the crash screen. */
enum crash_class {
    CRASH_UNCLASSIFIED,
    CRASH_DRIVER,
    CRASH_HARDWARE,
};

#define CRASH_CODE_DRIVER "09877KL"
#define CRASH_CODE_HARDWARE "56694MU"
#define CRASH_TITLE "oppsies... your pc seems to have crashed"

/* Rows needed to show the full-size picture together with all text. */
#define CRASH_FULL_LAYOUT_ROWS 38

struct crash_report {
    enum crash_class class;
    const char *summary;
    const char *detail;
};

/* Machine check and non-maskable interrupt vectors point at the CPU or motherboard; a fault whose instruction
   lies inside [driver_start, driver_end) points at a driver. Everything else stays unclassified. */
enum crash_class crash_classify_exception(uint64_t vector, uint64_t rip, uint64_t driver_start, uint64_t driver_end);

/* Error code text of a class, or NULL for an unclassified crash. */
const char *crash_class_code(enum crash_class class);

/* Draws the crash screen for a screen of the given height into the off-screen frame. */
void crash_draw(const struct crash_report *report, size_t rows);

/* Shows the crash screen, waits for a key and restarts the machine. */
__attribute__((noreturn)) void crash_show(const struct crash_report *report);

/* Linker-provided bounds of the code that belongs to drivers. */
uint64_t crash_driver_text_start(void);
uint64_t crash_driver_text_end(void);

#endif
