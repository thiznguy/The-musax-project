#ifndef MUSAX_CRASH_ART_H
#define MUSAX_CRASH_ART_H

#include <stddef.h>

struct crash_art {
    const char *const *rows;
    size_t row_count;
    size_t width;
};

extern const struct crash_art crash_art_full;
extern const struct crash_art crash_art_compact;

#endif
