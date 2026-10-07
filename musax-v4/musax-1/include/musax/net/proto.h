#ifndef MUSAX_NET_PROTO_H
#define MUSAX_NET_PROTO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Encoders and decoders for the packet formats of the network stack. They only touch the buffers they are given. */

#define ETH_HEADER_LEN 14
#define ETH_TYPE_IPV4 0x0800
#define ETH_TYPE_ARP 0x0806
#define ETH_FRAME_MAX 1518

uint32_t net_checksum_add(uint32_t sum, const void *data, size_t length);
uint16_t net_checksum_finish(uint32_t sum);
uint16_t net_checksum(const void *data, size_t length);

size_t eth_write_header(uint8_t *out, const uint8_t destination[6], const uint8_t source[6], uint16_t type);

#define ARP_PACKET_LEN 28
#define ARP_REQUEST 1
#define ARP_REPLY 2

struct arp_packet {
    uint16_t operation;
    uint8_t sender_mac[6];
    uint8_t sender_ip[4];
    uint8_t target_mac[6];
    uint8_t target_ip[4];
};

size_t arp_write(uint8_t *out, const struct arp_packet *packet);
bool arp_parse(const uint8_t *data, size_t length, struct arp_packet *packet);

#define IPV4_HEADER_LEN 20
#define IP_PROTOCOL_ICMP 1
#define IP_PROTOCOL_UDP 17

struct ipv4_header {
    uint8_t protocol;
    uint8_t ttl;
    uint16_t id;
    uint8_t source[4];
    uint8_t destination[4];
    size_t header_length;
    size_t total_length;
};

size_t ipv4_write_header(uint8_t *out, const uint8_t source[4], const uint8_t destination[4], uint8_t protocol,
                         size_t payload_length, uint16_t id);

/* Accepts only well-formed, unfragmented packets with a correct header checksum. */
bool ipv4_parse(const uint8_t *data, size_t length, struct ipv4_header *header);

#define ICMP_ECHO_REPLY 0
#define ICMP_ECHO_REQUEST 8
#define ICMP_HEADER_LEN 8

struct icmp_echo {
    uint8_t type;
    uint16_t id;
    uint16_t sequence;
    const uint8_t *payload;
    size_t payload_length;
};

size_t icmp_write_echo(uint8_t *out, uint8_t type, uint16_t id, uint16_t sequence, const uint8_t *payload,
                       size_t payload_length);
bool icmp_parse_echo(const uint8_t *data, size_t length, struct icmp_echo *echo);

#define UDP_HEADER_LEN 8

struct udp_datagram {
    uint16_t source_port;
    uint16_t destination_port;
    const uint8_t *payload;
    size_t payload_length;
};

size_t udp_write(uint8_t *out, const uint8_t source_ip[4], const uint8_t destination_ip[4], uint16_t source_port,
                 uint16_t destination_port, const uint8_t *payload, size_t payload_length);
bool udp_parse(const uint8_t *data, size_t length, const uint8_t source_ip[4], const uint8_t destination_ip[4],
               struct udp_datagram *datagram);

#define DHCP_CLIENT_PORT 68
#define DHCP_SERVER_PORT 67
#define DHCP_DISCOVER 1
#define DHCP_OFFER 2
#define DHCP_REQUEST 3
#define DHCP_ACK 5
#define DHCP_NAK 6
#define DHCP_MESSAGE_MIN 300

struct dhcp_result {
    uint8_t type;
    uint8_t your_ip[4];
    uint8_t server_id[4];
    uint8_t mask[4];
    uint8_t router[4];
    uint8_t dns[4];
    bool has_server_id;
    bool has_mask;
    bool has_router;
    bool has_dns;
    uint32_t lease_seconds;
};

size_t dhcp_write_discover(uint8_t *out, const uint8_t mac[6], uint32_t transaction_id);
size_t dhcp_write_request(uint8_t *out, const uint8_t mac[6], uint32_t transaction_id, const uint8_t requested_ip[4],
                          const uint8_t server_id[4]);
bool dhcp_parse(const uint8_t *data, size_t length, uint32_t transaction_id, const uint8_t mac[6],
                struct dhcp_result *result);

#define DNS_PORT 53

/* Returns the query length, or 0 when the name is empty, has an empty or over-long label, or does not fit. */
size_t dns_write_query(uint8_t *out, size_t capacity, uint16_t id, const char *name);

/* Finds the first IPv4 address in a response to the query with this id. */
bool dns_parse_response(const uint8_t *data, size_t length, uint16_t id, uint8_t address[4]);

#endif
