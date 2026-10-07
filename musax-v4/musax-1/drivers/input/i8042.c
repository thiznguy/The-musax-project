#include <stdbool.h>
#include <stdint.h>
#include <musax/arch/x86_64/io.h>
#include <musax/drivers/i8042.h>
#include <musax/drivers/ps2_keyboard.h>
#include <musax/drivers/ps2_mouse.h>

#define PS2_DATA_PORT 0x60
#define PS2_STATUS_PORT 0x64
#define PS2_COMMAND_PORT 0x64
#define PS2_STATUS_OUTPUT_FULL 0x01
#define PS2_STATUS_INPUT_FULL 0x02
#define PS2_STATUS_AUX_DATA 0x20
#define PS2_STATUS_NO_CONTROLLER 0xFF
#define PS2_MAX_BYTES_PER_POLL 32
#define PS2_WAIT_SPINS 200000

void i8042_poll(void)
{
    for (int i = 0; i < PS2_MAX_BYTES_PER_POLL; i++) {
        uint8_t status = inb(PS2_STATUS_PORT);
        uint8_t data;

        if (status == PS2_STATUS_NO_CONTROLLER || !(status & PS2_STATUS_OUTPUT_FULL))
            return;
        data = inb(PS2_DATA_PORT);
        if (status & PS2_STATUS_AUX_DATA)
            ps2_mouse_feed(data);
        else
            ps2_keyboard_feed(data);
    }
}

static bool wait_input_clear(void)
{
    for (int spins = 0; spins < PS2_WAIT_SPINS; spins++) {
        if (!(inb(PS2_STATUS_PORT) & PS2_STATUS_INPUT_FULL))
            return true;
    }
    return false;
}

bool i8042_write_command(uint8_t command)
{
    if (!wait_input_clear())
        return false;
    outb(PS2_COMMAND_PORT, command);
    return true;
}

bool i8042_write_data(uint8_t data)
{
    if (!wait_input_clear())
        return false;
    outb(PS2_DATA_PORT, data);
    return true;
}

bool i8042_read_data(uint8_t *data, bool *from_aux)
{
    for (int spins = 0; spins < PS2_WAIT_SPINS; spins++) {
        uint8_t status = inb(PS2_STATUS_PORT);

        if (status != PS2_STATUS_NO_CONTROLLER && (status & PS2_STATUS_OUTPUT_FULL)) {
            *from_aux = (status & PS2_STATUS_AUX_DATA) != 0;
            *data = inb(PS2_DATA_PORT);
            return true;
        }
    }
    return false;
}
