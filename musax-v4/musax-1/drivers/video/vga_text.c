#include <stddef.h>
#include <stdint.h>
#include <musax/arch/x86_64/io.h>
#include <musax/drivers/vga_text.h>

#define VGA_TEXT_BUFFER_ADDR 0xB8000UL
#define VGA_TAB_WIDTH 8
#define VGA_ATTRIBUTE_DEFAULT 0x07

#define BIOS_DATA_COLUMNS_ADDR 0x44AUL
#define BIOS_DATA_LAST_ROW_ADDR 0x484UL
#define VGA_DEFAULT_ROWS 25

#define CRTC_INDEX_PORT 0x3D4
#define CRTC_DATA_PORT 0x3D5
#define CRTC_CURSOR_START 0x0A
#define CRTC_CURSOR_LOCATION_HIGH 0x0E
#define CRTC_CURSOR_LOCATION_LOW 0x0F
#define CRTC_CURSOR_DISABLE 0x20

static volatile uint16_t *const text_buffer = (volatile uint16_t *)VGA_TEXT_BUFFER_ADDR;
static size_t screen_rows = VGA_DEFAULT_ROWS;
static size_t cursor_row;
static size_t cursor_column;

static uint16_t make_cell(uint8_t c, uint8_t attribute)
{
    return (uint16_t)(((uint16_t)attribute << 8) | c);
}

static void cursor_update(void)
{
    uint16_t position = (uint16_t)(cursor_row * VGA_COLUMNS + cursor_column);

    outb(CRTC_INDEX_PORT, CRTC_CURSOR_START);
    outb(CRTC_DATA_PORT, (uint8_t)(inb(CRTC_DATA_PORT) & ~CRTC_CURSOR_DISABLE));
    outb(CRTC_INDEX_PORT, CRTC_CURSOR_LOCATION_HIGH);
    outb(CRTC_DATA_PORT, (uint8_t)(position >> 8));
    outb(CRTC_INDEX_PORT, CRTC_CURSOR_LOCATION_LOW);
    outb(CRTC_DATA_PORT, (uint8_t)(position & 0xFF));
}

static void scroll_up(void)
{
    const size_t last_row_start = VGA_COLUMNS * (screen_rows - 1);

    for (size_t i = 0; i < last_row_start; i++)
        text_buffer[i] = text_buffer[i + VGA_COLUMNS];
    for (size_t i = last_row_start; i < VGA_COLUMNS * screen_rows; i++)
        text_buffer[i] = make_cell(' ', VGA_ATTRIBUTE_DEFAULT);
}

/* The BIOS data area sits at physical addresses below 4 KiB; the address goes through a variable so the compiler
   does not treat the read as a null-pointer access. */
static uint16_t bios_read16(uintptr_t address)
{
    volatile uintptr_t location = address;

    return *(volatile uint16_t *)location;
}

static uint8_t bios_read8(uintptr_t address)
{
    volatile uintptr_t location = address;

    return *(volatile uint8_t *)location;
}

void vga_text_init(void)
{
    uint16_t columns = bios_read16(BIOS_DATA_COLUMNS_ADDR);
    size_t rows = (size_t)bios_read8(BIOS_DATA_LAST_ROW_ADDR) + 1;

    if (columns == VGA_COLUMNS && rows >= VGA_DEFAULT_ROWS && rows <= VGA_MAX_ROWS)
        screen_rows = rows;
}

size_t vga_text_rows(void)
{
    return screen_rows;
}

void vga_text_clear(void)
{
    for (size_t i = 0; i < VGA_COLUMNS * screen_rows; i++)
        text_buffer[i] = make_cell(' ', VGA_ATTRIBUTE_DEFAULT);
    cursor_row = 0;
    cursor_column = 0;
    cursor_update();
}

void vga_text_put_cell(size_t row, size_t column, uint8_t c, uint8_t attribute)
{
    if (row < screen_rows && column < VGA_COLUMNS)
        text_buffer[row * VGA_COLUMNS + column] = make_cell(c, attribute);
}

void vga_text_set_cursor(size_t row, size_t column)
{
    cursor_row = row < screen_rows ? row : screen_rows - 1;
    cursor_column = column < VGA_COLUMNS ? column : VGA_COLUMNS - 1;
    cursor_update();
}

void vga_text_hide_cursor(void)
{
    outb(CRTC_INDEX_PORT, CRTC_CURSOR_START);
    outb(CRTC_DATA_PORT, (uint8_t)(inb(CRTC_DATA_PORT) | CRTC_CURSOR_DISABLE));
}

void vga_text_putc(char c)
{
    switch (c) {
    case '\n':
        cursor_column = 0;
        cursor_row++;
        break;
    case '\r':
        cursor_column = 0;
        break;
    case '\b':
        if (cursor_column > 0) {
            cursor_column--;
        } else if (cursor_row > 0) {
            cursor_row--;
            cursor_column = VGA_COLUMNS - 1;
        }
        text_buffer[cursor_row * VGA_COLUMNS + cursor_column] = make_cell(' ', VGA_ATTRIBUTE_DEFAULT);
        break;
    case '\t':
        do {
            vga_text_putc(' ');
        } while (cursor_column % VGA_TAB_WIDTH != 0);
        return;
    default:
        text_buffer[cursor_row * VGA_COLUMNS + cursor_column] = make_cell((uint8_t)c, VGA_ATTRIBUTE_DEFAULT);
        cursor_column++;
        break;
    }

    if (cursor_column >= VGA_COLUMNS) {
        cursor_column = 0;
        cursor_row++;
    }
    if (cursor_row >= screen_rows) {
        scroll_up();
        cursor_row = screen_rows - 1;
    }
    cursor_update();
}
