#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <musax/net/endian.h>
#include <musax/net/proto.h>

#define IPV4_VERSION_IHL 0x45
#define IPV4_FLAG_DONT_FRAGMENT 0x4000
#define IPV4_FRAGMENT_MASK 0x3FFF
#define IPV4_DEFAULT_TTL 64

size_t ipv4_write_header(uint8_t *out, const uint8_t source[4], const uint8_t destination[4], uint8_t protocol,
                         size_t payload_length, uint16_t id)
{
    out[0] = IPV4_VERSION_IHL;
    out[1] = 0;
    put16(out + 2, (uint16_t)(IPV4_HEADER_LEN + payload_length));
    put16(out + 4, id);
    put16(out + 6, IPV4_FLAG_DONT_FRAGMENT);
    out[8] = IPV4_DEFAULT_TTL;
    out[9] = protocol;
    put16(out + 10, 0);
    memcpy(out + 12, source, 4);
    memcpy(out + 16, destination, 4);
    put16(out + 10, net_checksum(out, IPV4_HEADER_LEN));
    return IPV4_HEADER_LEN;
}

bool ipv4_parse(const uint8_t *data, size_t length, struct ipv4_header *header)
{
    size_t header_length;
    size_t total_length;

    if (length < IPV4_HEADER_LEN || (data[0] >> 4) != 4)
        return false;
    header_length = (size_t)(data[0] & 0x0F) * 4;
    total_length = get16(data + 2);
    if (header_length < IPV4_HEADER_LEN || total_length < header_length || total_length > length)
        return false;
    if ((get16(data + 6) & IPV4_FRAGMENT_MASK) != 0)
        return false;
    if (net_checksum(data, header_length) != 0)
        return false;

    header->protocol = data[9];
    header->ttl = data[8];
    header->id = get16(data + 4);
    memcpy(header->source, data + 12, 4);
    memcpy(header->destination, data + 16, 4);
    header->header_length = header_length;
    header->total_length = total_length;
    return true;
}
