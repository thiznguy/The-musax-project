#include <stddef.h>
#include <stdint.h>
#include <musax/crash.h>

#define VECTOR_NMI 2
#define VECTOR_MACHINE_CHECK 18

enum crash_class crash_classify_exception(uint64_t vector, uint64_t rip, uint64_t driver_start, uint64_t driver_end)
{
    if (vector == VECTOR_MACHINE_CHECK || vector == VECTOR_NMI)
        return CRASH_HARDWARE;
    if (rip >= driver_start && rip < driver_end)
        return CRASH_DRIVER;
    return CRASH_UNCLASSIFIED;
}

const char *crash_class_code(enum crash_class class)
{
    switch (class) {
    case CRASH_DRIVER:
        return CRASH_CODE_DRIVER;
    case CRASH_HARDWARE:
        return CRASH_CODE_HARDWARE;
    default:
        return NULL;
    }
}
