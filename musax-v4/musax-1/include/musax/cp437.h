#ifndef MUSAX_CP437_H
#define MUSAX_CP437_H

#include <stddef.h>
#include <stdint.h>

/* Writes the UTF-8 encoding of an IBM code page 437 character into out and returns its length (1 to 3 bytes). */
size_t cp437_encode_utf8(uint8_t c, char out[4]);

#endif
