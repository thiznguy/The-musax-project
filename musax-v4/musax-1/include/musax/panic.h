#ifndef MUSAX_PANIC_H
#define MUSAX_PANIC_H

#include <musax/crash.h>

/* Stops the kernel and shows the crash screen with the formatted message as technical detail. */
__attribute__((noreturn, format(printf, 1, 2))) void kpanic(const char *format, ...);

/* Same as kpanic for a crash whose cause is already known, such as a driver that detected a fault. */
__attribute__((noreturn, format(printf, 2, 3))) void kpanic_class(enum crash_class class, const char *format, ...);

#endif
