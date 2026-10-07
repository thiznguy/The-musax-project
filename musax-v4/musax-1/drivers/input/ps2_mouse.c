#include <stdbool.h>
#include <stdint.h>
#include <musax/drivers/i8042.h>
#include <musax/drivers/ps2_keyboard.h>
#include <musax/drivers/ps2_mouse.h>

#define COMMAND_ENABLE_AUX 0xA8
#define COMMAND_READ_CONFIG 0x20
#define COMMAND_WRITE_CONFIG 0x60
#define COMMAND_WRITE_AUX 0xD4
#define CONFIG_AUX_IRQ 0x02
#define CONFIG_AUX_CLOCK_DISABLED 0x20

#define MOUSE_SET_DEFAULTS 0xF6
#define MOUSE_ENABLE_REPORTING 0xF4
#define MOUSE_ACK 0xFA

#define PACKET_ALWAYS_SET 0x08
#define PACKET_X_SIGN 0x10
#define PACKET_Y_SIGN 0x20
#define PACKET_OVERFLOW 0xC0
#define PACKET_BYTES 3
#define QUEUE_SIZE 64
#define ACK_READ_ATTEMPTS 8

static uint8_t packet[PACKET_BYTES];
static unsigned packet_length;
static struct mouse_event queue[QUEUE_SIZE];
static unsigned queue_head;
static unsigned queue_count;

static bool wait_for_ack(void)
{
    for (int attempt = 0; attempt < ACK_READ_ATTEMPTS; attempt++) {
        uint8_t data;
        bool from_aux;

        if (!i8042_read_data(&data, &from_aux))
            return false;
        if (!from_aux) {
            ps2_keyboard_feed(data);
            continue;
        }
        if (data == MOUSE_ACK)
            return true;
    }
    return false;
}

static bool send_to_mouse(uint8_t command)
{
    return i8042_write_command(COMMAND_WRITE_AUX) && i8042_write_data(command) && wait_for_ack();
}

bool ps2_mouse_init(void)
{
    uint8_t config;
    bool from_aux;

    packet_length = 0;
    queue_head = 0;
    queue_count = 0;

    if (!i8042_write_command(COMMAND_ENABLE_AUX))
        return false;
    if (!i8042_write_command(COMMAND_READ_CONFIG) || !i8042_read_data(&config, &from_aux))
        return false;
    config = (uint8_t)((config | CONFIG_AUX_IRQ) & ~CONFIG_AUX_CLOCK_DISABLED);
    if (!i8042_write_command(COMMAND_WRITE_CONFIG) || !i8042_write_data(config))
        return false;

    return send_to_mouse(MOUSE_SET_DEFAULTS) && send_to_mouse(MOUSE_ENABLE_REPORTING);
}

static void push_event(int dx, int dy, uint8_t buttons)
{
    unsigned tail;

    if (queue_count == QUEUE_SIZE)
        return;
    tail = (queue_head + queue_count) % QUEUE_SIZE;
    queue[tail].dx = dx;
    queue[tail].dy = dy;
    queue[tail].buttons = buttons;
    queue_count++;
}

void ps2_mouse_feed(uint8_t data)
{
    if (packet_length == 0 && !(data & PACKET_ALWAYS_SET))
        return;

    packet[packet_length++] = data;
    if (packet_length < PACKET_BYTES)
        return;
    packet_length = 0;

    if (packet[0] & PACKET_OVERFLOW)
        return;

    {
        int dx = packet[1] - ((packet[0] & PACKET_X_SIGN) ? 256 : 0);
        int dy = packet[2] - ((packet[0] & PACKET_Y_SIGN) ? 256 : 0);

        push_event(dx, -dy, (uint8_t)(packet[0] & 0x07));
    }
}

bool ps2_mouse_read(struct mouse_event *event)
{
    i8042_poll();
    if (queue_count == 0)
        return false;
    *event = queue[queue_head];
    queue_head = (queue_head + 1) % QUEUE_SIZE;
    queue_count--;
    return true;
}
