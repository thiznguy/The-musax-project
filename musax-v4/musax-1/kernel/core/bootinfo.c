#include <stddef.h>
#include <stdint.h>
#include <musax/boot/bootinfo.h>

static const struct bootinfo *boot_info;

void bootinfo_set(const struct bootinfo *info)
{
    boot_info = info;
}

const uint8_t *bootinfo_font(void)
{
    if (boot_info == NULL || boot_info->font_address == 0)
        return NULL;
    return (const uint8_t *)(uintptr_t)boot_info->font_address;
}
