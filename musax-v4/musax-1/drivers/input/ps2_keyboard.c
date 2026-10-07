#include <stdbool.h>
#include <stdint.h>
#include <musax/drivers/i8042.h>
#include <musax/drivers/ps2_keyboard.h>
#include <musax/keys.h>

#define KEY_QUEUE_SIZE 64

#define SCANCODE_RELEASE 0x80
#define SCANCODE_EXTENDED_PREFIX 0xE0
#define SCANCODE_CONTROL 0x1D
#define SCANCODE_ALT 0x38
#define SCANCODE_LEFT_SHIFT 0x2A
#define SCANCODE_RIGHT_SHIFT 0x36
#define SCANCODE_CAPS_LOCK 0x3A
#define SCANCODE_TABLE_SIZE 58

#define EXTENDED_KEYPAD_ENTER 0x1C
#define EXTENDED_HOME 0x47
#define EXTENDED_UP 0x48
#define EXTENDED_PAGE_UP 0x49
#define EXTENDED_LEFT 0x4B
#define EXTENDED_RIGHT 0x4D
#define EXTENDED_END 0x4F
#define EXTENDED_DOWN 0x50
#define EXTENDED_PAGE_DOWN 0x51
#define EXTENDED_DELETE 0x53

/* Scan code set 1 to ASCII, without and with a shift key held. */
static const char scancode_plain[SCANCODE_TABLE_SIZE] =
    "\0\0331234567890-=\b\tqwertyuiop[]\n\0asdfghjkl;'`\0\\zxcvbnm,./\0*\0 ";
static const char scancode_shifted[SCANCODE_TABLE_SIZE] =
    "\0\033!@#$%^&*()_+\b\tQWERTYUIOP{}\n\0ASDFGHJKL:\"~\0|ZXCVBNM<>?\0*\0 ";

static bool shift_held;
static bool control_held;
static bool alt_held;
static int key_queue[KEY_QUEUE_SIZE];
static unsigned key_queue_head;
static unsigned key_queue_count;
static bool caps_lock_on;
static bool extended_pending;

static bool is_lower_letter(char c)
{
    return c >= 'a' && c <= 'z';
}

static int translate_extended(uint8_t code, bool released)
{
    if (code == SCANCODE_CONTROL) {
        control_held = !released;
        return -1;
    }
    if (code == SCANCODE_ALT) {
        alt_held = !released;
        return -1;
    }
    if (released)
        return -1;

    switch (code) {
    case EXTENDED_KEYPAD_ENTER:
        return KEY_ENTER;
    case EXTENDED_HOME:
        return KEY_HOME;
    case EXTENDED_UP:
        return KEY_UP;
    case EXTENDED_PAGE_UP:
        return KEY_PAGE_UP;
    case EXTENDED_LEFT:
        return KEY_LEFT;
    case EXTENDED_RIGHT:
        return KEY_RIGHT;
    case EXTENDED_END:
        return KEY_END;
    case EXTENDED_DOWN:
        return KEY_DOWN;
    case EXTENDED_PAGE_DOWN:
        return KEY_PAGE_DOWN;
    case EXTENDED_DELETE:
        return KEY_DELETE;
    default:
        return -1;
    }
}

static int translate_character(uint8_t code)
{
    char c = scancode_plain[code];

    if (c == '\0')
        return -1;
    if (alt_held && (is_lower_letter(c) || (c >= '0' && c <= '9')))
        return KEY_ALT(c);
    if (is_lower_letter(c)) {
        if (control_held)
            return KEY_CTRL(c);
        return (shift_held != caps_lock_on) ? c - 'a' + 'A' : c;
    }
    if (shift_held)
        c = scancode_shifted[code];
    return (uint8_t)c;
}

static void queue_key(int key)
{
    if (key_queue_count == KEY_QUEUE_SIZE)
        return;
    key_queue[(key_queue_head + key_queue_count) % KEY_QUEUE_SIZE] = key;
    key_queue_count++;
}

void ps2_keyboard_init(void)
{
    i8042_poll();
    key_queue_head = 0;
    key_queue_count = 0;
}

void ps2_keyboard_feed(uint8_t code)
{
    bool released;
    int key;

    if (code == SCANCODE_EXTENDED_PREFIX) {
        extended_pending = true;
        return;
    }

    released = (code & SCANCODE_RELEASE) != 0;
    code &= (uint8_t)~SCANCODE_RELEASE;

    if (extended_pending) {
        extended_pending = false;
        key = translate_extended(code, released);
        if (key >= 0)
            queue_key(key);
        return;
    }

    if (code == SCANCODE_CONTROL) {
        control_held = !released;
        return;
    }
    if (code == SCANCODE_ALT) {
        alt_held = !released;
        return;
    }
    if (code == SCANCODE_LEFT_SHIFT || code == SCANCODE_RIGHT_SHIFT) {
        shift_held = !released;
        return;
    }
    if (code == SCANCODE_CAPS_LOCK) {
        if (!released)
            caps_lock_on = !caps_lock_on;
        return;
    }
    if (released || code >= SCANCODE_TABLE_SIZE)
        return;

    key = translate_character(code);
    if (key >= 0)
        queue_key(key);
}

int ps2_keyboard_read(void)
{
    int key;

    i8042_poll();
    if (key_queue_count == 0)
        return -1;
    key = key_queue[key_queue_head];
    key_queue_head = (key_queue_head + 1) % KEY_QUEUE_SIZE;
    key_queue_count--;
    return key;
}
