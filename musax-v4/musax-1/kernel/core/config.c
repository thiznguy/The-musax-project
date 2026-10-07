#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <libconf/conf.h>
#include <musax/config.h>
#include <musax/fs.h>

extern const char default_config_start[];
extern const char default_config_end[];

static struct config_entry entries[CONFIG_MAX_ENTRIES];
static size_t entry_count;

static struct config_entry *find_entry(const char *key)
{
    for (size_t i = 0; i < entry_count; i++) {
        if (strcmp(entries[i].key, key) == 0)
            return &entries[i];
    }
    return NULL;
}

static int collect(void *context, const char *key, const char *value)
{
    (void)context;
    config_set(key, value);
    return 0;
}

const char *config_default_text(size_t *length)
{
    *length = (size_t)(default_config_end - default_config_start);
    return default_config_start;
}

int config_set(const char *key, const char *value)
{
    struct config_entry *entry = find_entry(key);

    if (strlen(key) >= CONFIG_KEY_MAX || strlen(value) >= CONFIG_VALUE_MAX)
        return -1;
    if (entry == NULL) {
        if (entry_count == CONFIG_MAX_ENTRIES)
            return -1;
        entry = &entries[entry_count++];
        strcpy(entry->key, key);
    }
    strcpy(entry->value, value);
    return 0;
}

void config_load_defaults(void)
{
    size_t length;
    const char *text = config_default_text(&length);

    entry_count = 0;
    conf_parse(text, length, collect, NULL);
}

int config_reload(void)
{
    const struct fs_node *file = fs_lookup(CONFIG_FILE_PATH);
    char *text;
    int read;
    int applied;

    config_load_defaults();
    if (file == NULL || file->type != FS_NODE_FILE)
        return -1;

    text = malloc(file->file.size + 1);
    if (text == NULL)
        return -1;
    read = fs_read(file, 0, text, file->file.size);
    applied = read < 0 ? -1 : conf_parse(text, (size_t)read, collect, NULL);
    free(text);
    return applied;
}

int config_save(void)
{
    size_t total = 0;
    size_t position = 0;
    char *text;
    int result;

    for (size_t i = 0; i < entry_count; i++)
        total += strlen(entries[i].key) + strlen(entries[i].value) + 6;

    text = malloc(total + 1);
    if (text == NULL)
        return FS_ERR_NO_MEMORY;

    for (size_t i = 0; i < entry_count; i++) {
        size_t key_length = strlen(entries[i].key);
        size_t value_length = strlen(entries[i].value);

        memcpy(text + position, entries[i].key, key_length);
        position += key_length;
        memcpy(text + position, " = \"", 4);
        position += 4;
        memcpy(text + position, entries[i].value, value_length);
        position += value_length;
        text[position++] = '"';
        text[position++] = '\n';
    }

    result = fs_write_file(CONFIG_FILE_PATH, text, position);
    free(text);
    return result;
}

const char *config_get(const char *key, const char *fallback)
{
    const struct config_entry *entry = find_entry(key);

    return entry != NULL ? entry->value : fallback;
}

bool config_get_bool(const char *key, bool fallback)
{
    const char *value = config_get(key, NULL);

    if (value == NULL)
        return fallback;
    if (strcmp(value, "on") == 0 || strcmp(value, "true") == 0 || strcmp(value, "yes") == 0 ||
        strcmp(value, "1") == 0)
        return true;
    if (strcmp(value, "off") == 0 || strcmp(value, "false") == 0 || strcmp(value, "no") == 0 ||
        strcmp(value, "0") == 0)
        return false;
    return fallback;
}

long config_get_long(const char *key, long fallback)
{
    const char *value = config_get(key, NULL);
    char *end;
    long parsed;

    if (value == NULL || value[0] == '\0')
        return fallback;
    parsed = strtol(value, &end, 10);
    return *end == '\0' ? parsed : fallback;
}

size_t config_count(void)
{
    return entry_count;
}

const struct config_entry *config_entry_at(size_t index)
{
    return index < entry_count ? &entries[index] : NULL;
}
