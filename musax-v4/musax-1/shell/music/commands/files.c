#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <libpath/path.h>
#include <musax/config.h>
#include <musax/console.h>
#include <musax/fs.h>
#include <musax/kprintf.h>
#include <musax/programs/edit.h>
#include "../internal.h"

#define READ_CHUNK 256
#define TREE_PREFIX_MAX 128
#define TREE_DEPTH_MAX 32
#define DEFAULT_HOME "/users"

static bool has_flag(const char *argument, char flag)
{
    return argument[0] == '-' && argument[1] != '\0' && strchr(argument, flag) != NULL;
}

static bool is_option(const char *argument)
{
    return argument[0] == '-' && argument[1] != '\0';
}

static void print_directory_short(const struct fs_node *directory)
{
    size_t widest = 0;
    size_t cell;
    size_t per_row;
    size_t in_row = 0;

    for (const struct fs_node *child = directory->directory.first_child; child != NULL; child = child->next) {
        size_t width = strlen(child->name) + (child->type == FS_NODE_DIRECTORY ? 1 : 0);

        if (width > widest)
            widest = width;
    }

    cell = widest + 2;
    per_row = cell < CONSOLE_COLUMNS ? CONSOLE_COLUMNS / cell : 1;

    for (const struct fs_node *child = directory->directory.first_child; child != NULL; child = child->next) {
        size_t width = strlen(child->name) + (child->type == FS_NODE_DIRECTORY ? 1 : 0);

        kprintf("%s%s", child->name, child->type == FS_NODE_DIRECTORY ? "/" : "");
        if (++in_row == per_row || child->next == NULL) {
            console_putc('\n');
            in_row = 0;
        } else {
            for (size_t i = width; i < cell; i++)
                console_putc(' ');
        }
    }
}

static void print_node_long(const struct fs_node *node)
{
    bool is_directory = node->type == FS_NODE_DIRECTORY;
    size_t size = is_directory ? node->directory.child_count : node->file.size;

    kprintf("%c %8zu  %s%s\n", is_directory ? 'd' : '-', size, node->name, is_directory ? "/" : "");
}

int command_ls(int argc, char **argv)
{
    bool long_format = false;
    const char *target = ".";
    char path[FS_PATH_MAX];
    const struct fs_node *node;

    for (int i = 1; i < argc; i++) {
        if (is_option(argv[i]))
            long_format = long_format || has_flag(argv[i], 'l');
        else
            target = argv[i];
    }

    if (music_resolve(target, path) != 0) {
        kprintf("ls: %s: path too long\n", target);
        return 1;
    }
    node = fs_lookup(path);
    if (node == NULL) {
        music_report("ls", target, FS_ERR_NOT_FOUND);
        return 1;
    }

    if (node->type == FS_NODE_FILE) {
        print_node_long(node);
        return 0;
    }
    if (long_format) {
        for (const struct fs_node *child = node->directory.first_child; child != NULL; child = child->next)
            print_node_long(child);
    } else {
        print_directory_short(node);
    }
    return 0;
}

int command_cd(int argc, char **argv)
{
    const char *target = argc > 1 ? argv[1] : config_get("shell.home", DEFAULT_HOME);
    char path[FS_PATH_MAX];
    const struct fs_node *node;

    if (music_resolve(target, path) != 0) {
        kprintf("cd: %s: path too long\n", target);
        return 1;
    }
    node = fs_lookup(path);
    if (node == NULL) {
        music_report("cd", target, FS_ERR_NOT_FOUND);
        return 1;
    }
    if (node->type != FS_NODE_DIRECTORY) {
        music_report("cd", target, FS_ERR_NOT_DIRECTORY);
        return 1;
    }
    strcpy(music_cwd, path);
    return 0;
}

int command_pwd(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    kprintf("%s\n", music_cwd);
    return 0;
}

int command_mkdir(int argc, char **argv)
{
    bool parents = false;
    int status = 0;
    int paths = 0;

    for (int i = 1; i < argc; i++) {
        if (is_option(argv[i]))
            parents = parents || has_flag(argv[i], 'p');
    }

    for (int i = 1; i < argc; i++) {
        char path[FS_PATH_MAX];
        int error;

        if (is_option(argv[i]))
            continue;
        paths++;
        if (music_resolve(argv[i], path) != 0) {
            kprintf("mkdir: %s: path too long\n", argv[i]);
            status = 1;
            continue;
        }
        error = parents ? fs_create_directories(path) : fs_create_directory(path, NULL);
        if (error != FS_OK) {
            music_report("mkdir", argv[i], error);
            status = 1;
        }
    }

    if (paths == 0) {
        kprintf("usage: mkdir [-p] path...\n");
        return 1;
    }
    return status;
}

int command_touch(int argc, char **argv)
{
    int status = 0;

    if (argc < 2) {
        kprintf("usage: touch path...\n");
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        char path[FS_PATH_MAX];
        int error;

        if (music_resolve(argv[i], path) != 0) {
            kprintf("touch: %s: path too long\n", argv[i]);
            status = 1;
            continue;
        }
        if (fs_lookup(path) != NULL)
            continue;
        error = fs_create_file(path, NULL);
        if (error != FS_OK) {
            music_report("touch", argv[i], error);
            status = 1;
        }
    }
    return status;
}

int command_cat(int argc, char **argv)
{
    int status = 0;

    if (argc < 2) {
        kprintf("usage: cat path...\n");
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        char path[FS_PATH_MAX];
        const struct fs_node *node;
        size_t offset = 0;

        if (music_resolve(argv[i], path) != 0) {
            kprintf("cat: %s: path too long\n", argv[i]);
            status = 1;
            continue;
        }
        node = fs_lookup(path);
        if (node == NULL) {
            music_report("cat", argv[i], FS_ERR_NOT_FOUND);
            status = 1;
            continue;
        }
        if (node->type != FS_NODE_FILE) {
            music_report("cat", argv[i], FS_ERR_IS_DIRECTORY);
            status = 1;
            continue;
        }

        for (;;) {
            char chunk[READ_CHUNK];
            int count = fs_read(node, offset, chunk, sizeof(chunk));

            if (count <= 0)
                break;
            for (int j = 0; j < count; j++)
                console_putc(chunk[j]);
            offset += (size_t)count;
        }
    }
    return status;
}

int command_rm(int argc, char **argv)
{
    bool recursive = false;
    int status = 0;
    int paths = 0;

    for (int i = 1; i < argc; i++) {
        if (is_option(argv[i]))
            recursive = recursive || has_flag(argv[i], 'r');
    }

    for (int i = 1; i < argc; i++) {
        char path[FS_PATH_MAX];
        int error;

        if (is_option(argv[i]))
            continue;
        paths++;
        if (music_resolve(argv[i], path) != 0) {
            kprintf("rm: %s: path too long\n", argv[i]);
            status = 1;
            continue;
        }
        error = fs_remove(path, recursive);
        if (error != FS_OK) {
            music_report("rm", argv[i], error);
            status = 1;
        }
    }

    if (paths == 0) {
        kprintf("usage: rm [-r] path...\n");
        return 1;
    }
    return status;
}

int command_cp(int argc, char **argv)
{
    char source_path[FS_PATH_MAX];
    char destination_path[FS_PATH_MAX];
    const struct fs_node *source;
    const struct fs_node *destination;
    uint8_t *data;
    int read;
    int error;

    if (argc != 3) {
        kprintf("usage: cp source destination\n");
        return 1;
    }
    if (music_resolve(argv[1], source_path) != 0 || music_resolve(argv[2], destination_path) != 0) {
        kprintf("cp: path too long\n");
        return 1;
    }

    source = fs_lookup(source_path);
    if (source == NULL) {
        music_report("cp", argv[1], FS_ERR_NOT_FOUND);
        return 1;
    }
    if (source->type != FS_NODE_FILE) {
        music_report("cp", argv[1], FS_ERR_IS_DIRECTORY);
        return 1;
    }

    destination = fs_lookup(destination_path);
    if (destination != NULL && destination->type == FS_NODE_DIRECTORY) {
        size_t length = strlen(destination_path);

        if (length + 1 + strlen(source->name) + 1 > sizeof(destination_path)) {
            kprintf("cp: path too long\n");
            return 1;
        }
        if (length > 1)
            destination_path[length++] = '/';
        strcpy(destination_path + length, source->name);
    }
    if (strcmp(source_path, destination_path) == 0) {
        kprintf("cp: source and destination are the same file\n");
        return 1;
    }

    data = malloc(source->file.size > 0 ? source->file.size : 1);
    if (data == NULL) {
        music_report("cp", argv[1], FS_ERR_NO_MEMORY);
        return 1;
    }
    read = fs_read(source, 0, data, source->file.size);
    error = read < 0 ? read : fs_write_file(destination_path, data, (size_t)read);
    free(data);
    if (error != FS_OK) {
        music_report("cp", argv[2], error);
        return 1;
    }
    return 0;
}

int command_mv(int argc, char **argv)
{
    char source_path[FS_PATH_MAX];
    char destination_path[FS_PATH_MAX];
    int error;

    if (argc != 3) {
        kprintf("usage: mv source destination\n");
        return 1;
    }
    if (music_resolve(argv[1], source_path) != 0 || music_resolve(argv[2], destination_path) != 0) {
        kprintf("mv: path too long\n");
        return 1;
    }
    error = fs_move(source_path, destination_path);
    if (error != FS_OK) {
        music_report("mv", argv[1], error);
        return 1;
    }
    return 0;
}

static void print_tree(const struct fs_node *directory, char *prefix, size_t prefix_length, int depth)
{
    for (const struct fs_node *child = directory->directory.first_child; child != NULL; child = child->next) {
        bool last = child->next == NULL;

        kprintf("%s%s %s%s\n", prefix, last ? "`--" : "|--", child->name,
                child->type == FS_NODE_DIRECTORY ? "/" : "");

        if (child->type == FS_NODE_DIRECTORY && depth < TREE_DEPTH_MAX && prefix_length + 4 < TREE_PREFIX_MAX) {
            strcpy(prefix + prefix_length, last ? "    " : "|   ");
            print_tree(child, prefix, prefix_length + 4, depth + 1);
            prefix[prefix_length] = '\0';
        }
    }
}

int command_tree(int argc, char **argv)
{
    const char *target = argc > 1 ? argv[1] : ".";
    char path[FS_PATH_MAX];
    char prefix[TREE_PREFIX_MAX] = "";
    const struct fs_node *node;

    if (music_resolve(target, path) != 0) {
        kprintf("tree: %s: path too long\n", target);
        return 1;
    }
    node = fs_lookup(path);
    if (node == NULL) {
        music_report("tree", target, FS_ERR_NOT_FOUND);
        return 1;
    }
    if (node->type != FS_NODE_DIRECTORY) {
        music_report("tree", target, FS_ERR_NOT_DIRECTORY);
        return 1;
    }

    kprintf("%s\n", path);
    print_tree(node, prefix, 0, 0);
    return 0;
}

int command_fsinfo(int argc, char **argv)
{
    struct fs_statistics statistics;

    (void)argc;
    (void)argv;

    fs_get_statistics(&statistics);
    kprintf("%zu files, %zu directories, %zu bytes of file data (in memory, lost on reboot)\n", statistics.files,
            statistics.directories, statistics.bytes);
    return 0;
}

int command_edit(int argc, char **argv)
{
    char path[FS_PATH_MAX];
    int error;

    if (argc != 2) {
        kprintf("usage: edit path\n");
        return 1;
    }
    if (music_resolve(argv[1], path) != 0) {
        kprintf("edit: %s: path too long\n", argv[1]);
        return 1;
    }
    if (console_is_graphical()) {
        kprintf("edit: the editor needs the text console; leave the desktop first (Alt+Q)\n");
        return 1;
    }
    error = edit_run(path);
    if (error != FS_OK) {
        music_report("edit", argv[1], error);
        return 1;
    }
    return 0;
}
