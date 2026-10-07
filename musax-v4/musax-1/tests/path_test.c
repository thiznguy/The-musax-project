#include <string.h>
#include <libpath/path.h>
#include "check.h"

static const char *resolve(const char *base, const char *path)
{
    static char out[64];

    if (path_resolve(base, path, out, sizeof(out)) != 0)
        return "<error>";
    return out;
}

int main(void)
{
    char parent[64];

    CHECK_STR(resolve("/", ""), "/");
    CHECK_STR(resolve("/", "/"), "/");
    CHECK_STR(resolve("/users", "docs"), "/users/docs");
    CHECK_STR(resolve("/users", "/tools"), "/tools");
    CHECK_STR(resolve("/users/a", ".."), "/users");
    CHECK_STR(resolve("/users/a", "../b/./c"), "/users/b/c");
    CHECK_STR(resolve("/", ".."), "/");
    CHECK_STR(resolve("/a", "../../.."), "/");
    CHECK_STR(resolve("/a/b", "c///d/"), "/a/b/c/d");
    CHECK_STR(resolve("/a/b", "//c///d/"), "/c/d");
    CHECK_STR(resolve("/", "a/b/../../c"), "/c");
    CHECK_STR(resolve("/", "..hidden"), "/..hidden");
    CHECK_STR(resolve("/", ".../x"), "/.../x");

    {
        char tiny[8];

        CHECK(path_resolve("/", "abcdefghij", tiny, sizeof(tiny)) == -1);
        CHECK(path_resolve("/", "abcdef", tiny, sizeof(tiny)) == 0);
        CHECK(path_resolve("/", "abcdefg", tiny, sizeof(tiny)) == -1);
        CHECK(path_resolve("/", "a", tiny, 1) == -1);
    }

    CHECK_STR(path_basename("/a/b"), "b");
    CHECK_STR(path_basename("/a"), "a");
    CHECK_STR(path_basename("/"), "");

    CHECK(path_parent("/a/b", parent, sizeof(parent)) == 0);
    CHECK_STR(parent, "/a");
    CHECK(path_parent("/a", parent, sizeof(parent)) == 0);
    CHECK_STR(parent, "/");
    CHECK(path_parent("/", parent, sizeof(parent)) == 0);
    CHECK_STR(parent, "/");

    CHECK_REPORT("path");
}
