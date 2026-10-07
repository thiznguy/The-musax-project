#include <stdbool.h>
#include <stdint.h>
#include <musax/arch/x86_64/io.h>
#include <musax/arch/x86_64/paging.h>
#include <musax/drivers/display.h>
#include <musax/drivers/pci.h>

#define VBE_INDEX_PORT 0x1CE
#define VBE_DATA_PORT 0x1CF

#define VBE_REG_ID 0
#define VBE_REG_XRES 1
#define VBE_REG_YRES 2
#define VBE_REG_BPP 3
#define VBE_REG_ENABLE 4
#define VBE_REG_VIRT_WIDTH 6
#define VBE_REG_VIRT_HEIGHT 7
#define VBE_REG_X_OFFSET 8
#define VBE_REG_Y_OFFSET 9

#define VBE_ID_FIRST 0xB0C0
#define VBE_ID_LAST 0xB0CF
#define VBE_DISABLED 0x00
#define VBE_ENABLED 0x01
#define VBE_LINEAR_FRAMEBUFFER 0x40
#define VBE_BITS_PER_PIXEL 32
#define BYTES_PER_PIXEL 4

#define QEMU_VGA_VENDOR 0x1234
#define QEMU_VGA_DEVICE 0x1111
#define VIRTUALBOX_VGA_VENDOR 0x80EE
#define VIRTUALBOX_VGA_DEVICE 0xBEEF

static bool graphical;

static void vbe_write(uint16_t index, uint16_t value)
{
    outw(VBE_INDEX_PORT, index);
    outw(VBE_DATA_PORT, value);
}

static uint16_t vbe_read(uint16_t index)
{
    outw(VBE_INDEX_PORT, index);
    return inw(VBE_DATA_PORT);
}

bool display_enter_graphics(int width, int height, struct display_mode *mode)
{
    const struct pci_device *adapter = pci_find(QEMU_VGA_VENDOR, QEMU_VGA_DEVICE);
    uint16_t id;
    uint64_t framebuffer;

    if (adapter == NULL)
        adapter = pci_find(VIRTUALBOX_VGA_VENDOR, VIRTUALBOX_VGA_DEVICE);
    if (adapter == NULL)
        return false;

    id = vbe_read(VBE_REG_ID);
    if (id < VBE_ID_FIRST || id > VBE_ID_LAST)
        return false;

    framebuffer = pci_memory_bar(adapter, 0);
    if (framebuffer == 0 || framebuffer >= 0x100000000ULL)
        return false;
    pci_enable_device(adapter);

    vbe_write(VBE_REG_ENABLE, VBE_DISABLED);
    vbe_write(VBE_REG_XRES, (uint16_t)width);
    vbe_write(VBE_REG_YRES, (uint16_t)height);
    vbe_write(VBE_REG_BPP, VBE_BITS_PER_PIXEL);
    vbe_write(VBE_REG_VIRT_WIDTH, (uint16_t)width);
    vbe_write(VBE_REG_VIRT_HEIGHT, (uint16_t)height);
    vbe_write(VBE_REG_X_OFFSET, 0);
    vbe_write(VBE_REG_Y_OFFSET, 0);
    vbe_write(VBE_REG_ENABLE, VBE_ENABLED | VBE_LINEAR_FRAMEBUFFER);

    if (vbe_read(VBE_REG_XRES) != width || vbe_read(VBE_REG_YRES) != height) {
        vbe_write(VBE_REG_ENABLE, VBE_DISABLED);
        return false;
    }

    paging_set_uncached(framebuffer, (uint64_t)width * height * BYTES_PER_PIXEL);

    mode->framebuffer = (uint32_t *)(uintptr_t)framebuffer;
    mode->width = width;
    mode->height = height;
    mode->pitch_pixels = width;
    graphical = true;
    return true;
}

void display_leave_graphics(void)
{
    if (!graphical)
        return;
    vbe_write(VBE_REG_ENABLE, VBE_DISABLED);
    graphical = false;
}

bool display_is_graphical(void)
{
    return graphical;
}
