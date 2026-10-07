#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <musax/net/endian.h>
#include <musax/net/proto.h>

/* Checksum over the pseudo-header and the segment as stored; a segment with a correct checksum field yields 0. */
static uint16_t udp_sum(const uint8_t source_ip[4], const uint8_t destination_ip[4], const uint8_t *segment,
                        size_t length)
{
    uint8_t pseudo[12];
    uint32_t sum;

    memcpy(pseudo, source_ip, 4);
    memcpy(pseudo + 4, destination_ip, 4);
    pseudo[8] = 0;
    pseudo[9] = IP_PROTOCOL_UDP;
    put16(pseudo + 10, (uint16_t)length);

    sum = net_checksum_add(0, pseudo, sizeof(pseudo));
    sum = net_checksum_add(sum, segment, length);
    return net_checksum_finish(sum);
}

size_t udp_write(uint8_t *out, const uint8_t source_ip[4], const uint8_t destination_ip[4], uint16_t source_port,
                 uint16_t destination_port, const uint8_t *payload, size_t payload_length)
{
    size_t length = UDP_HEADER_LEN + payload_length;

    put16(out, source_port);
    put16(out + 2, destination_port);
    put16(out + 4, (uint16_t)length);
    put16(out + 6, 0);
    if (payload_length > 0)
        memcpy(out + UDP_HEADER_LEN, payload, payload_length);
    {
        uint16_t checksum = udp_sum(source_ip, destination_ip, out, length);

        put16(out + 6, checksum == 0 ? 0xFFFF : checksum);
    }
    return length;
}

bool udp_parse(const uint8_t *data, size_t length, const uint8_t source_ip[4], const uint8_t destination_ip[4],
               struct udp_datagram *datagram)
{
    size_t declared;

    if (length < UDP_HEADER_LEN)
        return false;
    declared = get16(data + 4);
    if (declared < UDP_HEADER_LEN || declared > length)
        return false;
    if (get16(data + 6) != 0 && udp_sum(source_ip, destination_ip, data, declared) != 0)
        return false;

    datagram->source_port = get16(data);
    datagram->destination_port = get16(data + 2);
    datagram->payload = data + UDP_HEADER_LEN;
    datagram->payload_length = declared - UDP_HEADER_LEN;
    return true;
}
