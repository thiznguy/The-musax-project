#ifndef LIBPATH_PATH_H
#define LIBPATH_PATH_H

#include <stddef.h>

/* Joins path onto the absolute directory base and normalizes "." and ".." components. Returns 0, or -1 when out is too small. */
int path_resolve(const char *base, const char *path, char *out, size_t capacity);

/* Last component of a normalized absolute path; empty for the root. */
const char *path_basename(const char *path);

/* Parent directory of a normalized absolute path; the root is its own parent. Returns 0, or -1 when out is too small. */
int path_parent(const char *path, char *out, size_t capacity);

#endif
