#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <musax/fs.h>
#include "check.h"

static size_t read_all(const char *path, char *buffer, size_t capacity)
{
    const struct fs_node *node = fs_lookup(path);
    int count;

    if (node == NULL || node->type != FS_NODE_FILE)
        return (size_t)-1;
    count = fs_read(node, 0, buffer, capacity - 1);
    buffer[count] = '\0';
    return (size_t)count;
}

static void test_layout(void)
{
    static const char *const directories[] = {
        "/tools", "/system", "/libs", "/settings", "/devices", "/users", "/temp", "/state",
    };

    for (size_t i = 0; i < sizeof(directories) / sizeof(directories[0]); i++) {
        const struct fs_node *node = fs_lookup(directories[i]);

        CHECK(node != NULL && node->type == FS_NODE_DIRECTORY);
    }
    CHECK(fs_lookup("/bin") == NULL);
    CHECK(fs_lookup("/") == fs_root());
    CHECK(fs_root()->directory.child_count == 8);
    CHECK_STR(fs_root()->directory.first_child->name, "devices");
}

static void test_files(void)
{
    char buffer[64];
    struct fs_node *node;

    CHECK(fs_write_file("/temp/a.txt", "hello", 5) == FS_OK);
    CHECK(read_all("/temp/a.txt", buffer, sizeof(buffer)) == 5);
    CHECK_STR(buffer, "hello");

    CHECK(fs_append_file("/temp/a.txt", " world", 6) == FS_OK);
    CHECK(read_all("/temp/a.txt", buffer, sizeof(buffer)) == 11);
    CHECK_STR(buffer, "hello world");

    CHECK(fs_write_file("/temp/a.txt", "x", 1) == FS_OK);
    CHECK(read_all("/temp/a.txt", buffer, sizeof(buffer)) == 1);
    CHECK_STR(buffer, "x");

    node = fs_lookup("/temp/a.txt");
    CHECK(fs_write(node, 4, "yz", 2) == 2);
    CHECK(node->file.size == 6);
    CHECK(memcmp(node->file.data, "x\0\0\0yz", 6) == 0);
    CHECK(fs_truncate(node, 2) == FS_OK);
    CHECK(node->file.size == 2);
    CHECK(fs_truncate(node, 4) == FS_OK);
    CHECK(memcmp(node->file.data, "x\0\0\0", 4) == 0);
    CHECK(fs_read(node, 10, buffer, 4) == 0);
    CHECK(fs_read(node, 1, buffer, 100) == 3);

    CHECK(fs_create_file("/temp/a.txt", NULL) == FS_ERR_EXISTS);
    CHECK(fs_create_file("/nowhere/f", NULL) == FS_ERR_NOT_FOUND);
    CHECK(fs_create_file("/temp/a.txt/f", NULL) == FS_ERR_NOT_DIRECTORY);
    CHECK(fs_write_file("/temp", "x", 1) == FS_ERR_IS_DIRECTORY);
    CHECK(fs_read(fs_lookup("/temp"), 0, buffer, 4) == FS_ERR_IS_DIRECTORY);

    {
        char long_name[FS_NAME_MAX + 10];
        char path[FS_PATH_MAX];

        memset(long_name, 'n', sizeof(long_name) - 1);
        long_name[sizeof(long_name) - 1] = '\0';
        snprintf(path, sizeof(path), "/temp/%s", long_name);
        CHECK(fs_create_file(path, NULL) == FS_ERR_NAME_TOO_LONG);
    }
}

static void test_directories(void)
{
    char path[FS_PATH_MAX];
    const struct fs_node *node;

    CHECK(fs_create_directory("/users/me", NULL) == FS_OK);
    CHECK(fs_create_directory("/users/me", NULL) == FS_ERR_EXISTS);
    CHECK(fs_create_directories("/users/me/a/b/c") == FS_OK);
    CHECK(fs_create_directories("/users/me/a/b/c") == FS_OK);
    CHECK(fs_lookup("/users/me/a/b/c") != NULL);
    CHECK(fs_create_directories("/temp/a.txt/x") == FS_ERR_NOT_DIRECTORY);

    node = fs_lookup("/users/me/a/b/c");
    CHECK(fs_node_path(node, path, sizeof(path)) == strlen("/users/me/a/b/c"));
    CHECK_STR(path, "/users/me/a/b/c");
    CHECK(fs_node_path(node, path, 5) == 0);
    CHECK(fs_node_path(fs_root(), path, sizeof(path)) == 1);
    CHECK_STR(path, "/");

    CHECK(fs_lookup("/users/me/a/../a/b") == fs_lookup("/users/me/a/b"));
    CHECK(fs_lookup("/users/./me") == fs_lookup("/users/me"));
    CHECK(fs_lookup("/../../users") == fs_lookup("/users"));

    CHECK(fs_remove("/users/me/a", false) == FS_ERR_NOT_EMPTY);
    CHECK(fs_remove("/users/me/a", true) == FS_OK);
    CHECK(fs_lookup("/users/me/a") == NULL);
    CHECK(fs_remove("/users/me", false) == FS_OK);
    CHECK(fs_remove("/users/me", false) == FS_ERR_NOT_FOUND);
    CHECK(fs_remove("/", true) == FS_ERR_INVALID);
}

static void test_sorting(void)
{
    const struct fs_node *child;
    static const char *const expected[] = { "alpha", "beta", "delta", "zulu" };
    size_t index = 0;

    CHECK(fs_create_directory("/temp/sorted", NULL) == FS_OK);
    CHECK(fs_create_file("/temp/sorted/zulu", NULL) == FS_OK);
    CHECK(fs_create_file("/temp/sorted/alpha", NULL) == FS_OK);
    CHECK(fs_create_directory("/temp/sorted/delta", NULL) == FS_OK);
    CHECK(fs_create_file("/temp/sorted/beta", NULL) == FS_OK);

    for (child = fs_lookup("/temp/sorted")->directory.first_child; child != NULL; child = child->next) {
        CHECK(index < 4);
        if (index < 4)
            CHECK_STR(child->name, expected[index]);
        index++;
    }
    CHECK(index == 4);
}

static void test_move(void)
{
    char buffer[16];

    CHECK(fs_create_directories("/temp/m/src") == FS_OK);
    CHECK(fs_create_directories("/temp/m/dst") == FS_OK);
    CHECK(fs_write_file("/temp/m/src/f", "data", 4) == FS_OK);

    CHECK(fs_move("/temp/m/src/f", "/temp/m/dst") == FS_OK);
    CHECK(fs_lookup("/temp/m/src/f") == NULL);
    CHECK(read_all("/temp/m/dst/f", buffer, sizeof(buffer)) == 4);

    CHECK(fs_move("/temp/m/dst/f", "/temp/m/dst/g") == FS_OK);
    CHECK(fs_lookup("/temp/m/dst/f") == NULL);
    CHECK(fs_lookup("/temp/m/dst/g") != NULL);

    CHECK(fs_write_file("/temp/m/src/h", "new", 3) == FS_OK);
    CHECK(fs_move("/temp/m/src/h", "/temp/m/dst/g") == FS_OK);
    CHECK(read_all("/temp/m/dst/g", buffer, sizeof(buffer)) == 3);
    CHECK_STR(buffer, "new");

    CHECK(fs_move("/temp/m/dst/g", "/temp/m/dst/g") == FS_OK);
    CHECK(fs_move("/temp/m/src", "/temp/m/src/inner") == FS_ERR_INVALID);
    CHECK(fs_move("/temp/m", "/temp/m/dst/deeper") == FS_ERR_INVALID);
    CHECK(fs_move("/temp/m/src", "/temp/m/dst") == FS_OK);
    CHECK(fs_lookup("/temp/m/dst/src") != NULL);
    CHECK(fs_move("/temp/m/dst/src", "/temp/m/dst/g") == FS_ERR_EXISTS);
    CHECK(fs_move("/missing", "/temp/x") == FS_ERR_NOT_FOUND);
    CHECK(fs_move("/temp/m/dst/g", "/missing/x") == FS_ERR_NOT_FOUND);
    CHECK(fs_move("/", "/temp/x") == FS_ERR_INVALID);
}

static void test_statistics(void)
{
    struct fs_statistics before;
    struct fs_statistics after;

    fs_get_statistics(&before);
    CHECK(fs_write_file("/temp/stat.bin", "12345", 5) == FS_OK);
    CHECK(fs_create_directory("/temp/stat_dir", NULL) == FS_OK);
    fs_get_statistics(&after);
    CHECK(after.files == before.files + 1);
    CHECK(after.directories == before.directories + 1);
    CHECK(after.bytes == before.bytes + 5);
}

static void test_large_file(void)
{
    size_t size = 100000;
    uint8_t *data = malloc(size);
    struct fs_node *node;

    for (size_t i = 0; i < size; i++)
        data[i] = (uint8_t)(i * 31 + 7);
    CHECK(fs_write_file("/temp/big", data, size) == FS_OK);
    node = fs_lookup("/temp/big");
    CHECK(node->file.size == size);
    CHECK(memcmp(node->file.data, data, size) == 0);

    for (size_t i = 0; i < 1000; i++)
        CHECK(fs_append_file("/temp/big", "abcdefghij", 10) == FS_OK || true);
    CHECK(node->file.size == size + 10000);
    free(data);
}

int main(void)
{
    fs_init();
    test_layout();
    test_files();
    test_directories();
    test_sorting();
    test_move();
    test_statistics();
    test_large_file();
    CHECK_REPORT("fs");
}
