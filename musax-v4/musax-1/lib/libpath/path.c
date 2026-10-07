#include <stddef.h>
#include <string.h>
#include <libpath/path.h>

static void drop_last_component(char *out, size_t *length)
{
    while (*length > 1 && out[*length - 1] != '/')
        (*length)--;
    if (*length > 1)
        (*length)--;
    out[*length] = '\0';
}

static int append_components(const char *path, char *out, size_t capacity, size_t *length)
{
    const char *cursor = path;

    for (;;) {
        const char *start;
        size_t count;
        size_t separator;

        while (*cursor == '/')
            cursor++;
        start = cursor;
        while (*cursor != '\0' && *cursor != '/')
            cursor++;
        count = (size_t)(cursor - start);
        if (count == 0)
            return 0;

        if (count == 1 && start[0] == '.')
            continue;
        if (count == 2 && start[0] == '.' && start[1] == '.') {
            drop_last_component(out, length);
            continue;
        }

        separator = *length > 1 ? 1 : 0;
        if (*length + separator + count + 1 > capacity)
            return -1;
        if (separator != 0)
            out[(*length)++] = '/';
        memcpy(out + *length, start, count);
        *length += count;
        out[*length] = '\0';
    }
}

int path_resolve(const char *base, const char *path, char *out, size_t capacity)
{
    size_t length = 1;

    if (capacity < 2)
        return -1;
    out[0] = '/';
    out[1] = '\0';

    if (path[0] != '/' && append_components(base, out, capacity, &length) != 0)
        return -1;
    return append_components(path, out, capacity, &length);
}

const char *path_basename(const char *path)
{
    const char *slash = strrchr(path, '/');

    return slash == NULL ? path : slash + 1;
}

int path_parent(const char *path, char *out, size_t capacity)
{
    size_t length = strlen(path);

    if (length + 1 > capacity)
        return -1;
    memcpy(out, path, length + 1);
    if (length > 1)
        drop_last_component(out, &length);
    return 0;
}
