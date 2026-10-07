#ifndef MUSAX_KEYS_H
#define MUSAX_KEYS_H

/* Key codes returned by the console: ASCII values plus non-ASCII keys from 0x100 upward. */
#define KEY_BACKSPACE 0x08
#define KEY_TAB 0x09
#define KEY_ENTER 0x0A
#define KEY_ESCAPE 0x1B

#define KEY_CTRL(letter) ((letter) & 0x1F)

#define KEY_UP 0x100
#define KEY_DOWN 0x101
#define KEY_LEFT 0x102
#define KEY_RIGHT 0x103
#define KEY_HOME 0x104
#define KEY_END 0x105
#define KEY_PAGE_UP 0x106
#define KEY_PAGE_DOWN 0x107
#define KEY_DELETE 0x108

/* Alt held together with a lower-case letter or digit. */
#define KEY_ALT_BASE 0x200
#define KEY_ALT(c) (KEY_ALT_BASE + (c))

#endif
