#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <musax/net/endian.h>
#include <musax/net/proto.h>

#define ARP_HARDWARE_ETHERNET 1
#define ARP_HARDWARE_LENGTH 6
#define ARP_PROTOCOL_LENGTH 4

size_t arp_write(uint8_t *out, const struct arp_packet *packet)
{
    put16(out, ARP_HARDWARE_ETHERNET);
    put16(out + 2, ETH_TYPE_IPV4);
    out[4] = ARP_HARDWARE_LENGTH;
    out[5] = ARP_PROTOCOL_LENGTH;
    put16(out + 6, packet->operation);
    memcpy(out + 8, packet->sender_mac, 6);
    memcpy(out + 14, packet->sender_ip, 4);
    memcpy(out + 18, packet->target_mac, 6);
    memcpy(out + 24, packet->target_ip, 4);
    return ARP_PACKET_LEN;
}

bool arp_parse(const uint8_t *data, size_t length, struct arp_packet *packet)
{
    if (length < ARP_PACKET_LEN)
        return false;
    if (get16(data) != ARP_HARDWARE_ETHERNET || get16(data + 2) != ETH_TYPE_IPV4 ||
        data[4] != ARP_HARDWARE_LENGTH || data[5] != ARP_PROTOCOL_LENGTH)
        return false;

    packet->operation = get16(data + 6);
    memcpy(packet->sender_mac, data + 8, 6);
    memcpy(packet->sender_ip, data + 14, 4);
    memcpy(packet->target_mac, data + 18, 6);
    memcpy(packet->target_ip, data + 24, 4);
    return packet->operation == ARP_REQUEST || packet->operation == ARP_REPLY;
}
