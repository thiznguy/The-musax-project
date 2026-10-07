#ifndef MUSAX_GUI_THEME_H
#define MUSAX_GUI_THEME_H

#include <stddef.h>
#include <musax/gui/gfx.h>

/* A desktop background: a vertical gradient from top to bottom. */
struct theme {
    const char *name;
    color_t top;
    color_t bottom;
};

size_t theme_count(void);
const struct theme *theme_at(size_t index);

/* Index of the theme with this name; 0 (the default theme) when the name is unknown. */
size_t theme_index_by_name(const char *name);

#endif
