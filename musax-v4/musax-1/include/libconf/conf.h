#ifndef LIBCONF_CONF_H
#define LIBCONF_CONF_H

#include <stddef.h>

#define CONF_KEY_MAX 64

/* Called for every key/value pair; a non-zero return stops parsing. Keys inside a [section] arrive as "section.key". */
typedef int (*conf_handler)(void *context, const char *key, const char *value);

/* Parses "key = value" text with "#" or ";" comments, [section] headers and optionally double-quoted values.
   Returns the number of pairs delivered, or -1 when the handler stopped parsing. Malformed lines are skipped. */
int conf_parse(const char *text, size_t length, conf_handler handler, void *context);

#endif
