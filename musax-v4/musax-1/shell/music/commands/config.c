#include <string.h>
#include <musax/config.h>
#include <musax/fs.h>
#include <musax/kprintf.h>
#include "../internal.h"

static int print_all(void)
{
    for (size_t i = 0; i < config_count(); i++) {
        const struct config_entry *entry = config_entry_at(i);

        kprintf("%s = %s\n", entry->key, entry->value);
    }
    return 0;
}

int command_config(int argc, char **argv)
{
    if (argc == 1)
        return print_all();

    if (strcmp(argv[1], "get") == 0 && argc == 3) {
        const char *value = config_get(argv[2], NULL);

        if (value == NULL) {
            kprintf("config: no such key: %s\n", argv[2]);
            return 1;
        }
        kprintf("%s\n", value);
        return 0;
    }

    if (strcmp(argv[1], "set") == 0 && argc == 4) {
        if (config_set(argv[2], argv[3]) != 0) {
            kprintf("config: cannot set %s\n", argv[2]);
            return 1;
        }
        return 0;
    }

    if (strcmp(argv[1], "save") == 0 && argc == 2) {
        int error = config_save();

        if (error != FS_OK) {
            music_report("config", CONFIG_FILE_PATH, error);
            return 1;
        }
        kprintf("saved %s\n", CONFIG_FILE_PATH);
        return 0;
    }

    if (strcmp(argv[1], "reload") == 0 && argc == 2) {
        int applied = config_reload();

        if (applied < 0) {
            kprintf("config: could not read %s, using defaults\n", CONFIG_FILE_PATH);
            return 1;
        }
        kprintf("reloaded %d keys from %s\n", applied, CONFIG_FILE_PATH);
        return 0;
    }

    kprintf("usage: config [get key | set key value | save | reload]\n");
    return 1;
}
