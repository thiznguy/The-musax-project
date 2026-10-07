#include <stdint.h>
#include <string.h>
#include <musax/net/net.h>
#include <musax/net/proto.h>
#include "check.h"

/* Independent reference implementation of the internet checksum, used to cross-check net_checksum. */
static uint16_t reference_checksum(const uint8_t *data, size_t length)
{
    uint32_t sum = 0;

    for (size_t i = 0; i + 1 < length; i += 2)
        sum += (uint32_t)data[i] << 8 | data[i + 1];
    if (length % 2)
        sum += (uint32_t)data[length - 1] << 8;
    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);
    return (uint16_t)~sum;
}

static void test_checksum(void)
{
    static const uint8_t rfc1071[] = { 0x00, 0x01, 0xf2, 0x03, 0xf4, 0xf5, 0xf6, 0xf7 };
    uint8_t random_data[1500];
    uint32_t state = 7;

    CHECK(net_checksum(rfc1071, sizeof(rfc1071)) == 0x220D);
    CHECK(net_checksum("", 0) == 0xFFFF);
    CHECK(net_checksum("\x01", 1) == 0xFEFF);

    for (size_t length = 0; length < sizeof(random_data); length += 7) {
        for (size_t i = 0; i < length; i++) {
            state = state * 1664525U + 1013904223U;
            random_data[i] = (uint8_t)(state >> 16);
        }
        CHECK(net_checksum(random_data, length) == reference_checksum(random_data, length));
    }
}

static void test_ipv4(void)
{
    /* The header from the Wikipedia IPv4 checksum example: checksum b861. */
    static const uint8_t source[4] = { 192, 168, 0, 1 };
    static const uint8_t destination[4] = { 192, 168, 0, 199 };
    static const uint8_t expected[] = { 0x45, 0x00, 0x00, 0x73, 0x00, 0x00, 0x40, 0x00, 0x40, 0x11,
                                        0xb8, 0x61, 0xc0, 0xa8, 0x00, 0x01, 0xc0, 0xa8, 0x00, 0xc7 };
    uint8_t packet[200] = { 0 };
    struct ipv4_header header;

    CHECK(ipv4_write_header(packet, source, destination, IP_PROTOCOL_UDP, 0x73 - 20, 0) == 20);
    CHECK(memcmp(packet, expected, sizeof(expected)) == 0);
    CHECK(ipv4_parse(packet, 0x73, &header));
    CHECK(header.protocol == IP_PROTOCOL_UDP && header.ttl == 64 && header.total_length == 0x73);
    CHECK(memcmp(header.source, source, 4) == 0 && memcmp(header.destination, destination, 4) == 0);

    CHECK(!ipv4_parse(packet, 0x72, &header));
    packet[8] ^= 1;
    CHECK(!ipv4_parse(packet, 0x73, &header));
    packet[8] ^= 1;
    packet[6] = 0x20;
    CHECK(!ipv4_parse(packet, 0x73, &header));
    packet[6] = 0x40;
    packet[0] = 0x65;
    CHECK(!ipv4_parse(packet, 0x73, &header));
    CHECK(!ipv4_parse(packet, 10, &header));
}

static void test_icmp(void)
{
    uint8_t packet[64];
    uint8_t payload[32];
    struct icmp_echo echo;
    size_t length;

    CHECK(icmp_write_echo(packet, ICMP_ECHO_REQUEST, 0, 0, NULL, 0) == 8);
    CHECK(packet[0] == 8 && packet[2] == 0xF7 && packet[3] == 0xFF);

    for (size_t i = 0; i < sizeof(payload); i++)
        payload[i] = (uint8_t)('a' + i % 26);
    length = icmp_write_echo(packet, ICMP_ECHO_REPLY, 0x4D58, 7, payload, sizeof(payload));
    CHECK(length == 40);
    CHECK(icmp_parse_echo(packet, length, &echo));
    CHECK(echo.type == ICMP_ECHO_REPLY && echo.id == 0x4D58 && echo.sequence == 7);
    CHECK(echo.payload_length == 32 && memcmp(echo.payload, payload, 32) == 0);

    packet[20] ^= 0x10;
    CHECK(!icmp_parse_echo(packet, length, &echo));
    packet[20] ^= 0x10;
    packet[0] = 3;
    CHECK(!icmp_parse_echo(packet, length, &echo));
}

static void test_udp(void)
{
    static const uint8_t source[4] = { 10, 0, 2, 15 };
    static const uint8_t destination[4] = { 10, 0, 2, 3 };
    static const uint8_t other[4] = { 10, 0, 2, 4 };
    uint8_t segment[64];
    struct udp_datagram datagram;
    size_t length = udp_write(segment, source, destination, 49152, 53, (const uint8_t *)"hello", 5);

    CHECK(length == 13);
    CHECK(udp_parse(segment, length, source, destination, &datagram));
    CHECK(datagram.source_port == 49152 && datagram.destination_port == 53);
    CHECK(datagram.payload_length == 5 && memcmp(datagram.payload, "hello", 5) == 0);

    CHECK(!udp_parse(segment, length, source, other, &datagram));
    segment[9] ^= 1;
    CHECK(!udp_parse(segment, length, source, destination, &datagram));
    segment[9] ^= 1;
    segment[6] = 0;
    segment[7] = 0;
    CHECK(udp_parse(segment, length, source, destination, &datagram));
    CHECK(!udp_parse(segment, 7, source, destination, &datagram));
    segment[5] = 99;
    CHECK(!udp_parse(segment, length, source, destination, &datagram));
}

static void test_arp(void)
{
    struct arp_packet request = { .operation = ARP_REQUEST,
                                  .sender_mac = { 0x52, 0x54, 0x00, 0x12, 0x34, 0x56 },
                                  .sender_ip = { 10, 0, 2, 15 },
                                  .target_mac = { 0 },
                                  .target_ip = { 10, 0, 2, 2 } };
    static const uint8_t expected[] = { 0x00, 0x01, 0x08, 0x00, 0x06, 0x04, 0x00, 0x01, 0x52, 0x54, 0x00, 0x12,
                                        0x34, 0x56, 0x0a, 0x00, 0x02, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                        0x0a, 0x00, 0x02, 0x02 };
    uint8_t packet[ARP_PACKET_LEN];
    struct arp_packet parsed;

    CHECK(arp_write(packet, &request) == ARP_PACKET_LEN);
    CHECK(memcmp(packet, expected, sizeof(expected)) == 0);
    CHECK(arp_parse(packet, sizeof(packet), &parsed));
    CHECK(parsed.operation == ARP_REQUEST && memcmp(parsed.target_ip, request.target_ip, 4) == 0);
    CHECK(!arp_parse(packet, 27, &parsed));
    packet[7] = 3;
    CHECK(!arp_parse(packet, sizeof(packet), &parsed));
    packet[7] = 1;
    packet[4] = 8;
    CHECK(!arp_parse(packet, sizeof(packet), &parsed));
}

static void test_ethernet(void)
{
    static const uint8_t destination[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
    static const uint8_t source[6] = { 0x52, 0x54, 0x00, 0x12, 0x34, 0x56 };
    uint8_t frame[14];

    CHECK(eth_write_header(frame, destination, source, ETH_TYPE_ARP) == 14);
    CHECK(frame[12] == 0x08 && frame[13] == 0x06 && memcmp(frame + 6, source, 6) == 0);
}

static void build_dhcp_reply(uint8_t *message, size_t *length, const uint8_t mac[6], uint32_t xid, uint8_t type)
{
    size_t position = 240;

    memset(message, 0, 400);
    message[0] = 2;
    message[1] = 1;
    message[2] = 6;
    message[4] = (uint8_t)(xid >> 24);
    message[5] = (uint8_t)(xid >> 16);
    message[6] = (uint8_t)(xid >> 8);
    message[7] = (uint8_t)xid;
    memcpy(message + 16, (const uint8_t[]){ 10, 0, 2, 15 }, 4);
    memcpy(message + 28, mac, 6);
    memcpy(message + 236, (const uint8_t[]){ 0x63, 0x82, 0x53, 0x63 }, 4);

    memcpy(message + position, (const uint8_t[]){ 53, 1, type }, 3);
    position += 3;
    memcpy(message + position, (const uint8_t[]){ 54, 4, 10, 0, 2, 2 }, 6);
    position += 6;
    memcpy(message + position, (const uint8_t[]){ 1, 4, 255, 255, 255, 0 }, 6);
    position += 6;
    memcpy(message + position, (const uint8_t[]){ 3, 4, 10, 0, 2, 2 }, 6);
    position += 6;
    memcpy(message + position, (const uint8_t[]){ 6, 8, 10, 0, 2, 3, 8, 8, 8, 8 }, 10);
    position += 10;
    memcpy(message + position, (const uint8_t[]){ 51, 4, 0, 0, 0x0E, 0x10 }, 6);
    position += 6;
    message[position++] = 255;
    *length = position;
}

static void test_dhcp(void)
{
    static const uint8_t mac[6] = { 0x52, 0x54, 0x00, 0x12, 0x34, 0x56 };
    static const uint8_t other_mac[6] = { 0x52, 0x54, 0x00, 0x12, 0x34, 0x57 };
    uint8_t message[512];
    struct dhcp_result result;
    size_t length;

    length = dhcp_write_discover(message, mac, 0xCAFEF00D);
    CHECK(length == DHCP_MESSAGE_MIN);
    CHECK(message[0] == 1 && message[1] == 1 && message[2] == 6);
    CHECK(memcmp(message + 4, "\xca\xfe\xf0\x0d", 4) == 0);
    CHECK(memcmp(message + 28, mac, 6) == 0);
    CHECK(memcmp(message + 236, "\x63\x82\x53\x63", 4) == 0);
    CHECK(message[240] == 53 && message[241] == 1 && message[242] == DHCP_DISCOVER);
    CHECK((message[10] & 0x80) != 0);

    length = dhcp_write_request(message, mac, 0xCAFEF00D, (const uint8_t[]){ 10, 0, 2, 15 },
                                (const uint8_t[]){ 10, 0, 2, 2 });
    CHECK(message[242] == DHCP_REQUEST);
    CHECK(message[243] == 50 && memcmp(message + 245, "\x0a\x00\x02\x0f", 4) == 0);
    CHECK(message[249] == 54 && memcmp(message + 251, "\x0a\x00\x02\x02", 4) == 0);

    build_dhcp_reply(message, &length, mac, 0xCAFEF00D, DHCP_OFFER);
    CHECK(dhcp_parse(message, length, 0xCAFEF00D, mac, &result));
    CHECK(result.type == DHCP_OFFER);
    CHECK(memcmp(result.your_ip, "\x0a\x00\x02\x0f", 4) == 0);
    CHECK(result.has_server_id && memcmp(result.server_id, "\x0a\x00\x02\x02", 4) == 0);
    CHECK(result.has_mask && memcmp(result.mask, "\xff\xff\xff\x00", 4) == 0);
    CHECK(result.has_router && memcmp(result.router, "\x0a\x00\x02\x02", 4) == 0);
    CHECK(result.has_dns && memcmp(result.dns, "\x0a\x00\x02\x03", 4) == 0);
    CHECK(result.lease_seconds == 3600);

    CHECK(!dhcp_parse(message, length, 0x12345678, mac, &result));
    CHECK(!dhcp_parse(message, length, 0xCAFEF00D, other_mac, &result));
    CHECK(!dhcp_parse(message, 100, 0xCAFEF00D, mac, &result));
    message[0] = 1;
    CHECK(!dhcp_parse(message, length, 0xCAFEF00D, mac, &result));
    message[0] = 2;
    message[236] = 0;
    CHECK(!dhcp_parse(message, length, 0xCAFEF00D, mac, &result));
    message[236] = 0x63;
    CHECK(!dhcp_parse(message, 252, 0xCAFEF00D, mac, &result));

    build_dhcp_reply(message, &length, mac, 1, DHCP_ACK);
    CHECK(dhcp_parse(message, length, 1, mac, &result) && result.type == DHCP_ACK);
    message[length - 8] = 200;
    CHECK(!dhcp_parse(message, length, 1, mac, &result) || result.type == DHCP_ACK);
}

static void test_dns(void)
{
    static const uint8_t expected_query[] = { 0x12, 0x34, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                              0x07, 'e', 'x', 'a', 'm', 'p', 'l', 'e', 0x03, 'c', 'o', 'm', 0x00,
                                              0x00, 0x01, 0x00, 0x01 };
    uint8_t query[128];
    uint8_t response[128];
    uint8_t address[4];
    size_t length;
    size_t position;

    CHECK(dns_write_query(query, sizeof(query), 0x1234, "example.com") == sizeof(expected_query));
    CHECK(memcmp(query, expected_query, sizeof(expected_query)) == 0);
    CHECK(dns_write_query(query, sizeof(query), 1, "") == 0);
    CHECK(dns_write_query(query, sizeof(query), 1, "a..b") == 0);
    CHECK(dns_write_query(query, sizeof(query), 1, ".a") == 0);
    CHECK(dns_write_query(query, 20, 1, "example.com") == 0);
    {
        char long_label[80];

        memset(long_label, 'x', 64);
        long_label[64] = '\0';
        CHECK(dns_write_query(query, sizeof(query), 1, long_label) == 0);
    }

    memcpy(response, expected_query, sizeof(expected_query));
    response[2] = 0x81;
    response[3] = 0x80;
    response[7] = 2;
    position = sizeof(expected_query);
    memcpy(response + position, (const uint8_t[]){ 0xC0, 0x0C, 0x00, 0x05, 0x00, 0x01, 0, 0, 0, 60, 0x00, 0x02, 0xC0, 0x0C },
           14);
    position += 14;
    memcpy(response + position, (const uint8_t[]){ 0xC0, 0x0C, 0x00, 0x01, 0x00, 0x01, 0, 0, 0, 60, 0x00, 0x04, 93, 184, 216, 34 },
           16);
    position += 16;
    length = position;

    CHECK(dns_parse_response(response, length, 0x1234, address));
    CHECK(memcmp(address, "\x5d\xb8\xd8\x22", 4) == 0);
    CHECK(!dns_parse_response(response, length, 0x4321, address));
    CHECK(!dns_parse_response(response, length - 3, 0x1234, address));
    response[3] = 0x83;
    CHECK(!dns_parse_response(response, length, 0x1234, address));
    response[3] = 0x80;
    response[2] = 0x01;
    CHECK(!dns_parse_response(response, length, 0x1234, address));
    CHECK(!dns_parse_response(response, 5, 0x1234, address));
}

static void test_addresses(void)
{
    uint8_t address[4];
    char text[32];

    CHECK(net_parse_ip("10.0.2.15", address) && memcmp(address, "\x0a\x00\x02\x0f", 4) == 0);
    CHECK(net_parse_ip("255.255.255.255", address));
    CHECK(net_parse_ip("0.0.0.0", address));
    CHECK(!net_parse_ip("256.1.1.1", address));
    CHECK(!net_parse_ip("1.2.3", address));
    CHECK(!net_parse_ip("1.2.3.4.5", address));
    CHECK(!net_parse_ip("1.2.3.", address));
    CHECK(!net_parse_ip("a.b.c.d", address));
    CHECK(!net_parse_ip("1.2.3.4 ", address));
    CHECK(!net_parse_ip("", address));
    CHECK(!net_parse_ip("example.com", address));
    CHECK(!net_parse_ip("1234.1.1.1", address));

    net_format_ip((const uint8_t[]){ 192, 168, 1, 7 }, text, sizeof(text));
    CHECK_STR(text, "192.168.1.7");
    net_format_mac((const uint8_t[]){ 0x52, 0x54, 0x00, 0xAB, 0x0C, 0xFF }, text, sizeof(text));
    CHECK_STR(text, "52:54:00:ab:0c:ff");
}

int main(void)
{
    test_checksum();
    test_ipv4();
    test_icmp();
    test_udp();
    test_arp();
    test_ethernet();
    test_dhcp();
    test_dns();
    test_addresses();
    CHECK_REPORT("net");
}
