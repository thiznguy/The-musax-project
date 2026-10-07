#include <stdarg.h>
#include <stdio.h>
#include <musax/crash.h>
#include <musax/panic.h>

#define PANIC_MESSAGE_MAX 128

static void panic_with(enum crash_class class, const char *format, va_list args)
{
    static char message[PANIC_MESSAGE_MAX];
    struct crash_report report;

    vsnprintf(message, sizeof(message), format, args);
    report.class = class;
    report.summary = "kernel panic";
    report.detail = message;
    crash_show(&report);
}

void kpanic(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    panic_with(CRASH_UNCLASSIFIED, format, args);
    va_end(args);
    __builtin_unreachable();
}

void kpanic_class(enum crash_class class, const char *format, ...)
{
    va_list args;

    va_start(args, format);
    panic_with(class, format, args);
    va_end(args);
    __builtin_unreachable();
}
