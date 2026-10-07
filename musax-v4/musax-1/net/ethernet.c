#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <musax/net/endian.h>
#include <musax/net/proto.h>

size_t eth_write_header(uint8_t *out, const uint8_t destination[6], const uint8_t source[6], uint16_t type)
{
    memcpy(out, destination, 6);
    memcpy(out + 6, source, 6);
    put16(out + 12, type);
    return ETH_HEADER_LEN;
}
