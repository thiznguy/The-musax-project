#ifndef MUSAX_CONFIG_H
#define MUSAX_CONFIG_H

#include <stdbool.h>
#include <stddef.h>

#define CONFIG_MAX_ENTRIES 64
#define CONFIG_KEY_MAX 48
#define CONFIG_VALUE_MAX 96
#define CONFIG_FILE_PATH "/settings/musax.conf"

struct config_entry {
    char key[CONFIG_KEY_MAX];
    char value[CONFIG_VALUE_MAX];
};

/* Loads the defaults compiled into the kernel image. */
void config_load_defaults(void);

/* Resets to the defaults, then applies the overrides found in CONFIG_FILE_PATH. Returns the number of keys applied or -1. */
int config_reload(void);
int config_save(void);

int config_set(const char *key, const char *value);
const char *config_get(const char *key, const char *fallback);
bool config_get_bool(const char *key, bool fallback);
long config_get_long(const char *key, long fallback);
size_t config_count(void);
const struct config_entry *config_entry_at(size_t index);

/* The default configuration text embedded in the kernel image. */
const char *config_default_text(size_t *length);

#endif
