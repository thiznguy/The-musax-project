#include <stddef.h>
#include <stdint.h>
#include <musax/net/proto.h>

uint32_t net_checksum_add(uint32_t sum, const void *data, size_t length)
{
    const uint8_t *bytes = data;

    for (; length >= 2; length -= 2, bytes += 2)
        sum += (uint32_t)((bytes[0] << 8) | bytes[1]);
    if (length == 1)
        sum += (uint32_t)(bytes[0] << 8);
    return sum;
}

uint16_t net_checksum_finish(uint32_t sum)
{
    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);
    return (uint16_t)~sum;
}

uint16_t net_checksum(const void *data, size_t length)
{
    return net_checksum_finish(net_checksum_add(0, data, length));
}
