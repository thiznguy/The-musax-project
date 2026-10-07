#ifndef MUSAX_FS_H
#define MUSAX_FS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define FS_NAME_MAX 63
#define FS_PATH_MAX 256

#define FS_OK 0
#define FS_ERR_NOT_FOUND (-1)
#define FS_ERR_EXISTS (-2)
#define FS_ERR_NOT_DIRECTORY (-3)
#define FS_ERR_IS_DIRECTORY (-4)
#define FS_ERR_NOT_EMPTY (-5)
#define FS_ERR_NO_MEMORY (-6)
#define FS_ERR_INVALID (-7)
#define FS_ERR_NAME_TOO_LONG (-8)

enum fs_node_type {
    FS_NODE_FILE = 1,
    FS_NODE_DIRECTORY = 2,
};

/* One file or directory of the in-memory filesystem; siblings are linked in name order. */
struct fs_node {
    char name[FS_NAME_MAX + 1];
    enum fs_node_type type;
    struct fs_node *parent;
    struct fs_node *next;
    union {
        struct {
            uint8_t *data;
            size_t size;
            size_t capacity;
        } file;
        struct {
            struct fs_node *first_child;
            size_t child_count;
        } directory;
    };
};

struct fs_statistics {
    size_t files;
    size_t directories;
    size_t bytes;
};

/* Creates the root directory and the standard musax directory layout. */
void fs_init(void);
void fs_create_standard_layout(void);

struct fs_node *fs_root(void);

/* Every path argument is an absolute path; "." and ".." components are normalized. */
struct fs_node *fs_lookup(const char *path);
int fs_create_file(const char *path, struct fs_node **created);
int fs_create_directory(const char *path, struct fs_node **created);
int fs_create_directories(const char *path);
int fs_remove(const char *path, bool recursive);
int fs_move(const char *source, const char *destination);

/* fs_read and fs_write return the number of bytes transferred or a negative error. */
int fs_read(const struct fs_node *node, size_t offset, void *buffer, size_t length);
int fs_write(struct fs_node *node, size_t offset, const void *data, size_t length);
int fs_truncate(struct fs_node *node, size_t size);
int fs_write_file(const char *path, const void *data, size_t length);
int fs_append_file(const char *path, const void *data, size_t length);

/* Writes the absolute path of node into buffer; returns its length, or 0 when buffer is too small. */
size_t fs_node_path(const struct fs_node *node, char *buffer, size_t capacity);
void fs_get_statistics(struct fs_statistics *statistics);
const char *fs_error_message(int error);

#endif
