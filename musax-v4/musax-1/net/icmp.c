#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <musax/net/endian.h>
#include <musax/net/proto.h>

size_t icmp_write_echo(uint8_t *out, uint8_t type, uint16_t id, uint16_t sequence, const uint8_t *payload,
                       size_t payload_length)
{
    out[0] = type;
    out[1] = 0;
    put16(out + 2, 0);
    put16(out + 4, id);
    put16(out + 6, sequence);
    if (payload_length > 0)
        memcpy(out + ICMP_HEADER_LEN, payload, payload_length);
    put16(out + 2, net_checksum(out, ICMP_HEADER_LEN + payload_length));
    return ICMP_HEADER_LEN + payload_length;
}

bool icmp_parse_echo(const uint8_t *data, size_t length, struct icmp_echo *echo)
{
    if (length < ICMP_HEADER_LEN)
        return false;
    if ((data[0] != ICMP_ECHO_REQUEST && data[0] != ICMP_ECHO_REPLY) || data[1] != 0)
        return false;
    if (net_checksum(data, length) != 0)
        return false;

    echo->type = data[0];
    echo->id = get16(data + 4);
    echo->sequence = get16(data + 6);
    echo->payload = data + ICMP_HEADER_LEN;
    echo->payload_length = length - ICMP_HEADER_LEN;
    return true;
}
