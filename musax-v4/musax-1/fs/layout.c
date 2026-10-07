#include <stddef.h>
#include <musax/fs.h>

/* Top-level directories of musax-1 and the conventional Unix directory each one replaces:
   tools=bin, system=sbin, libs=lib, settings=etc, devices=dev, users=home, temp=tmp, state=var. */
static const char *const standard_directories[] = {
    "/tools", "/system", "/libs", "/settings", "/devices", "/users", "/temp", "/state",
};

void fs_create_standard_layout(void)
{
    for (size_t i = 0; i < sizeof(standard_directories) / sizeof(standard_directories[0]); i++)
        fs_create_directory(standard_directories[i], NULL);
}
