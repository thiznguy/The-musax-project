#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <libpath/path.h>
#include <musax/fs.h>

#define FS_MIN_FILE_CAPACITY 64

static struct fs_node *root;

static struct fs_node *node_create(const char *name, size_t name_length, enum fs_node_type type)
{
    struct fs_node *node = calloc(1, sizeof(*node));

    if (node == NULL)
        return NULL;
    memcpy(node->name, name, name_length);
    node->name[name_length] = '\0';
    node->type = type;
    return node;
}

static void node_destroy(struct fs_node *node)
{
    if (node->type == FS_NODE_DIRECTORY) {
        struct fs_node *child = node->directory.first_child;

        while (child != NULL) {
            struct fs_node *next = child->next;

            node_destroy(child);
            child = next;
        }
    } else {
        free(node->file.data);
    }
    free(node);
}

static void link_child(struct fs_node *parent, struct fs_node *node)
{
    struct fs_node **link = &parent->directory.first_child;

    while (*link != NULL && strcmp((*link)->name, node->name) < 0)
        link = &(*link)->next;
    node->next = *link;
    *link = node;
    node->parent = parent;
    parent->directory.child_count++;
}

static void unlink_child(struct fs_node *node)
{
    struct fs_node **link = &node->parent->directory.first_child;

    while (*link != node)
        link = &(*link)->next;
    *link = node->next;
    node->parent->directory.child_count--;
    node->next = NULL;
}

static struct fs_node *find_child(const struct fs_node *directory, const char *name, size_t name_length)
{
    for (struct fs_node *child = directory->directory.first_child; child != NULL; child = child->next) {
        if (strlen(child->name) == name_length && memcmp(child->name, name, name_length) == 0)
            return child;
    }
    return NULL;
}

static int normalize(const char *path, char *normalized)
{
    return path_resolve("/", path, normalized, FS_PATH_MAX) == 0 ? FS_OK : FS_ERR_INVALID;
}

static struct fs_node *lookup_normalized(const char *normalized)
{
    struct fs_node *node = root;
    const char *cursor = normalized;

    for (;;) {
        const char *start;

        while (*cursor == '/')
            cursor++;
        if (*cursor == '\0')
            return node;
        start = cursor;
        while (*cursor != '\0' && *cursor != '/')
            cursor++;
        if (node->type != FS_NODE_DIRECTORY)
            return NULL;
        node = find_child(node, start, (size_t)(cursor - start));
        if (node == NULL)
            return NULL;
    }
}

static int create_node(const char *path, enum fs_node_type type, struct fs_node **created)
{
    char normalized[FS_PATH_MAX];
    char parent_path[FS_PATH_MAX];
    struct fs_node *parent;
    struct fs_node *node;
    const char *name;
    size_t name_length;
    int error = normalize(path, normalized);

    if (error != FS_OK)
        return error;
    if (strcmp(normalized, "/") == 0)
        return FS_ERR_EXISTS;
    if (path_parent(normalized, parent_path, sizeof(parent_path)) != 0)
        return FS_ERR_INVALID;

    parent = lookup_normalized(parent_path);
    if (parent == NULL)
        return FS_ERR_NOT_FOUND;
    if (parent->type != FS_NODE_DIRECTORY)
        return FS_ERR_NOT_DIRECTORY;

    name = path_basename(normalized);
    name_length = strlen(name);
    if (name_length > FS_NAME_MAX)
        return FS_ERR_NAME_TOO_LONG;
    if (find_child(parent, name, name_length) != NULL)
        return FS_ERR_EXISTS;

    node = node_create(name, name_length, type);
    if (node == NULL)
        return FS_ERR_NO_MEMORY;
    link_child(parent, node);
    if (created != NULL)
        *created = node;
    return FS_OK;
}

static int reserve(struct fs_node *node, size_t needed)
{
    size_t capacity;
    uint8_t *data;

    if (node->file.capacity >= needed)
        return FS_OK;
    capacity = node->file.capacity > FS_MIN_FILE_CAPACITY ? node->file.capacity : FS_MIN_FILE_CAPACITY;
    while (capacity < needed) {
        if (capacity > SIZE_MAX / 2)
            return FS_ERR_NO_MEMORY;
        capacity *= 2;
    }
    data = realloc(node->file.data, capacity);
    if (data == NULL)
        return FS_ERR_NO_MEMORY;
    node->file.data = data;
    node->file.capacity = capacity;
    return FS_OK;
}

static bool is_descendant_or_self(const struct fs_node *node, const struct fs_node *ancestor)
{
    for (; node != NULL; node = node->parent) {
        if (node == ancestor)
            return true;
    }
    return false;
}

static void accumulate(const struct fs_node *node, struct fs_statistics *statistics)
{
    if (node->type == FS_NODE_DIRECTORY) {
        statistics->directories++;
        for (const struct fs_node *child = node->directory.first_child; child != NULL; child = child->next)
            accumulate(child, statistics);
    } else {
        statistics->files++;
        statistics->bytes += node->file.size;
    }
}

void fs_init(void)
{
    root = node_create("", 0, FS_NODE_DIRECTORY);
    fs_create_standard_layout();
}

struct fs_node *fs_root(void)
{
    return root;
}

struct fs_node *fs_lookup(const char *path)
{
    char normalized[FS_PATH_MAX];

    if (root == NULL || normalize(path, normalized) != FS_OK)
        return NULL;
    return lookup_normalized(normalized);
}

int fs_create_file(const char *path, struct fs_node **created)
{
    return create_node(path, FS_NODE_FILE, created);
}

int fs_create_directory(const char *path, struct fs_node **created)
{
    return create_node(path, FS_NODE_DIRECTORY, created);
}

int fs_create_directories(const char *path)
{
    char normalized[FS_PATH_MAX];
    struct fs_node *node = root;
    const char *cursor;
    int error = normalize(path, normalized);

    if (error != FS_OK)
        return error;

    cursor = normalized;
    for (;;) {
        const char *start;
        size_t name_length;
        struct fs_node *child;

        while (*cursor == '/')
            cursor++;
        if (*cursor == '\0')
            return FS_OK;
        start = cursor;
        while (*cursor != '\0' && *cursor != '/')
            cursor++;
        name_length = (size_t)(cursor - start);
        if (name_length > FS_NAME_MAX)
            return FS_ERR_NAME_TOO_LONG;

        child = find_child(node, start, name_length);
        if (child == NULL) {
            child = node_create(start, name_length, FS_NODE_DIRECTORY);
            if (child == NULL)
                return FS_ERR_NO_MEMORY;
            link_child(node, child);
        } else if (child->type != FS_NODE_DIRECTORY) {
            return FS_ERR_NOT_DIRECTORY;
        }
        node = child;
    }
}

int fs_remove(const char *path, bool recursive)
{
    struct fs_node *node = fs_lookup(path);

    if (node == NULL)
        return FS_ERR_NOT_FOUND;
    if (node == root)
        return FS_ERR_INVALID;
    if (node->type == FS_NODE_DIRECTORY && node->directory.child_count > 0 && !recursive)
        return FS_ERR_NOT_EMPTY;
    unlink_child(node);
    node_destroy(node);
    return FS_OK;
}

int fs_move(const char *source, const char *destination)
{
    char destination_normalized[FS_PATH_MAX];
    char parent_path[FS_PATH_MAX];
    struct fs_node *node = fs_lookup(source);
    struct fs_node *target_parent;
    struct fs_node *existing;
    const char *name;
    size_t name_length;
    int error;

    if (node == NULL)
        return FS_ERR_NOT_FOUND;
    if (node == root)
        return FS_ERR_INVALID;

    error = normalize(destination, destination_normalized);
    if (error != FS_OK)
        return error;

    target_parent = lookup_normalized(destination_normalized);
    if (target_parent != NULL && target_parent->type == FS_NODE_DIRECTORY) {
        name = node->name;
    } else {
        if (path_parent(destination_normalized, parent_path, sizeof(parent_path)) != 0)
            return FS_ERR_INVALID;
        target_parent = lookup_normalized(parent_path);
        if (target_parent == NULL)
            return FS_ERR_NOT_FOUND;
        if (target_parent->type != FS_NODE_DIRECTORY)
            return FS_ERR_NOT_DIRECTORY;
        name = path_basename(destination_normalized);
    }

    name_length = strlen(name);
    if (name_length == 0)
        return FS_ERR_INVALID;
    if (name_length > FS_NAME_MAX)
        return FS_ERR_NAME_TOO_LONG;
    if (is_descendant_or_self(target_parent, node))
        return FS_ERR_INVALID;

    existing = find_child(target_parent, name, name_length);
    if (existing == node)
        return FS_OK;
    if (existing != NULL) {
        if (existing->type != FS_NODE_FILE || node->type != FS_NODE_FILE)
            return FS_ERR_EXISTS;
        unlink_child(existing);
        node_destroy(existing);
    }

    unlink_child(node);
    memcpy(node->name, name, name_length);
    node->name[name_length] = '\0';
    link_child(target_parent, node);
    return FS_OK;
}

int fs_read(const struct fs_node *node, size_t offset, void *buffer, size_t length)
{
    if (node->type != FS_NODE_FILE)
        return FS_ERR_IS_DIRECTORY;
    if (offset >= node->file.size)
        return 0;
    if (length > node->file.size - offset)
        length = node->file.size - offset;
    if (length > INT32_MAX)
        length = INT32_MAX;
    memcpy(buffer, node->file.data + offset, length);
    return (int)length;
}

int fs_write(struct fs_node *node, size_t offset, const void *data, size_t length)
{
    size_t end;
    int error;

    if (node->type != FS_NODE_FILE)
        return FS_ERR_IS_DIRECTORY;
    if (length > INT32_MAX || offset > SIZE_MAX - length)
        return FS_ERR_INVALID;
    end = offset + length;

    error = reserve(node, end);
    if (error != FS_OK)
        return error;
    if (offset > node->file.size)
        memset(node->file.data + node->file.size, 0, offset - node->file.size);
    if (length > 0)
        memcpy(node->file.data + offset, data, length);
    if (end > node->file.size)
        node->file.size = end;
    return (int)length;
}

int fs_truncate(struct fs_node *node, size_t size)
{
    int error;

    if (node->type != FS_NODE_FILE)
        return FS_ERR_IS_DIRECTORY;
    if (size > node->file.size) {
        error = reserve(node, size);
        if (error != FS_OK)
            return error;
        memset(node->file.data + node->file.size, 0, size - node->file.size);
    }
    node->file.size = size;
    return FS_OK;
}

int fs_write_file(const char *path, const void *data, size_t length)
{
    struct fs_node *node = fs_lookup(path);
    int result;

    if (node == NULL) {
        result = fs_create_file(path, &node);
        if (result != FS_OK)
            return result;
    } else if (node->type != FS_NODE_FILE) {
        return FS_ERR_IS_DIRECTORY;
    }

    node->file.size = 0;
    result = fs_write(node, 0, data, length);
    return result < 0 ? result : FS_OK;
}

int fs_append_file(const char *path, const void *data, size_t length)
{
    struct fs_node *node = fs_lookup(path);
    int result;

    if (node == NULL) {
        result = fs_create_file(path, &node);
        if (result != FS_OK)
            return result;
    } else if (node->type != FS_NODE_FILE) {
        return FS_ERR_IS_DIRECTORY;
    }

    result = fs_write(node, node->file.size, data, length);
    return result < 0 ? result : FS_OK;
}

size_t fs_node_path(const struct fs_node *node, char *buffer, size_t capacity)
{
    size_t length = 0;
    size_t position;

    if (node == root) {
        if (capacity < 2)
            return 0;
        buffer[0] = '/';
        buffer[1] = '\0';
        return 1;
    }

    for (const struct fs_node *walk = node; walk != root; walk = walk->parent)
        length += 1 + strlen(walk->name);
    if (length + 1 > capacity)
        return 0;

    position = length;
    buffer[position] = '\0';
    for (const struct fs_node *walk = node; walk != root; walk = walk->parent) {
        size_t name_length = strlen(walk->name);

        position -= name_length;
        memcpy(buffer + position, walk->name, name_length);
        buffer[--position] = '/';
    }
    return length;
}

void fs_get_statistics(struct fs_statistics *statistics)
{
    memset(statistics, 0, sizeof(*statistics));
    if (root != NULL)
        accumulate(root, statistics);
}

const char *fs_error_message(int error)
{
    switch (error) {
    case FS_OK:
        return "success";
    case FS_ERR_NOT_FOUND:
        return "no such file or directory";
    case FS_ERR_EXISTS:
        return "already exists";
    case FS_ERR_NOT_DIRECTORY:
        return "not a directory";
    case FS_ERR_IS_DIRECTORY:
        return "is a directory";
    case FS_ERR_NOT_EMPTY:
        return "directory not empty";
    case FS_ERR_NO_MEMORY:
        return "out of memory";
    case FS_ERR_INVALID:
        return "invalid argument";
    case FS_ERR_NAME_TOO_LONG:
        return "name too long";
    default:
        return "unknown error";
    }
}
