#include <stddef.h>
#include <string.h>
#include <musax/gui/theme.h>

static const struct theme themes[] = {
    { "ocean", RGB(10, 40, 90), RGB(30, 125, 175) },
    { "midnight", RGB(8, 8, 24), RGB(46, 34, 90) },
    { "forest", RGB(10, 48, 32), RGB(64, 136, 84) },
    { "plum", RGB(48, 16, 64), RGB(158, 74, 138) },
    { "ember", RGB(70, 20, 10), RGB(226, 116, 44) },
    { "slate", RGB(36, 42, 52), RGB(96, 110, 128) },
    { "rose", RGB(92, 30, 62), RGB(238, 146, 166) },
    { "mono", RGB(14, 14, 14), RGB(52, 52, 52) },
};

size_t theme_count(void)
{
    return sizeof(themes) / sizeof(themes[0]);
}

const struct theme *theme_at(size_t index)
{
    return index < theme_count() ? &themes[index] : &themes[0];
}

size_t theme_index_by_name(const char *name)
{
    for (size_t i = 0; i < theme_count(); i++) {
        if (strcmp(themes[i].name, name) == 0)
            return i;
    }
    return 0;
}
