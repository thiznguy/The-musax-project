#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <musax/console.h>
#include <musax/drivers/e1000.h>
#include <musax/drivers/timer.h>
#include <musax/net/endian.h>
#include <musax/net/net.h>
#include <musax/net/proto.h>

#define MAX_FRAMES_PER_POLL 16
#define UDP_HANDLERS 4
#define ARP_RETRIES 3
#define ARP_RETRY_MS 300
#define DHCP_ATTEMPTS 3
#define DHCP_STEP_MS 1500
#define DNS_ATTEMPTS 2
#define PING_PAYLOAD 32
#define PING_ID 0x4D58
#define EPHEMERAL_PORT_BASE 49152
#define PAYLOAD_MAX (ETH_FRAME_MAX - ETH_HEADER_LEN - IPV4_HEADER_LEN)

static const uint8_t broadcast_mac[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
static const uint8_t broadcast_ip[4] = { 255, 255, 255, 255 };
static const uint8_t zero_ip[4] = { 0, 0, 0, 0 };

typedef void (*udp_handler)(const uint8_t source_ip[4], uint16_t source_port, const uint8_t *payload, size_t length);

static struct netdev device;
static bool present;
static struct net_config config;
static struct arp_entry arp_cache[NET_ARP_CACHE_SIZE];
static size_t arp_count;
static size_t arp_next_slot;
static uint16_t ip_identification;

static struct {
    uint16_t port;
    udp_handler handler;
} udp_handlers[UDP_HANDLERS];

static uint8_t rx_frame[ETH_FRAME_MAX];
static uint8_t tx_frame[ETH_FRAME_MAX];

static volatile bool ping_reply_seen;
static uint16_t ping_expected_sequence;

static bool dhcp_reply_seen;
static struct dhcp_result dhcp_reply;
static uint32_t dhcp_transaction;

static bool dns_reply_seen;
static uint8_t dns_answer[4];
static uint16_t dns_transaction;

static bool same_ip(const uint8_t *a, const uint8_t *b)
{
    return memcmp(a, b, 4) == 0;
}

static bool on_link(const uint8_t destination[4])
{
    for (int i = 0; i < 4; i++) {
        if ((destination[i] & config.mask[i]) != (config.ip[i] & config.mask[i]))
            return false;
    }
    return true;
}

static void arp_remember(const uint8_t ip[4], const uint8_t mac[6])
{
    for (size_t i = 0; i < arp_count; i++) {
        if (same_ip(arp_cache[i].ip, ip)) {
            memcpy(arp_cache[i].mac, mac, 6);
            return;
        }
    }
    if (arp_count < NET_ARP_CACHE_SIZE) {
        memcpy(arp_cache[arp_count].ip, ip, 4);
        memcpy(arp_cache[arp_count].mac, mac, 6);
        arp_count++;
        return;
    }
    memcpy(arp_cache[arp_next_slot].ip, ip, 4);
    memcpy(arp_cache[arp_next_slot].mac, mac, 6);
    arp_next_slot = (arp_next_slot + 1) % NET_ARP_CACHE_SIZE;
}

static const uint8_t *arp_lookup(const uint8_t ip[4])
{
    for (size_t i = 0; i < arp_count; i++) {
        if (same_ip(arp_cache[i].ip, ip))
            return arp_cache[i].mac;
    }
    return NULL;
}

static bool send_frame(size_t length)
{
    if (length < 60) {
        memset(tx_frame + length, 0, 60 - length);
        length = 60;
    }
    return device.send(tx_frame, length) == 0;
}

static void send_arp(uint16_t operation, const uint8_t target_mac[6], const uint8_t target_ip[4])
{
    struct arp_packet packet;
    size_t length;

    packet.operation = operation;
    memcpy(packet.sender_mac, device.mac, 6);
    memcpy(packet.sender_ip, config.ip, 4);
    memcpy(packet.target_mac, operation == ARP_REQUEST ? (const uint8_t[6]){ 0 } : target_mac, 6);
    memcpy(packet.target_ip, target_ip, 4);

    length = eth_write_header(tx_frame, operation == ARP_REQUEST ? broadcast_mac : target_mac, device.mac,
                              ETH_TYPE_ARP);
    length += arp_write(tx_frame + length, &packet);
    send_frame(length);
}

static bool send_ip(const uint8_t destination_mac[6], const uint8_t source_ip[4], const uint8_t destination_ip[4],
                    uint8_t protocol, const uint8_t *payload, size_t payload_length)
{
    size_t length;

    if (payload_length > PAYLOAD_MAX)
        return false;
    length = eth_write_header(tx_frame, destination_mac, device.mac, ETH_TYPE_IPV4);
    length += ipv4_write_header(tx_frame + length, source_ip, destination_ip, protocol, payload_length,
                                ip_identification++);
    memcpy(tx_frame + length, payload, payload_length);
    return send_frame(length + payload_length);
}

static void send_ping_reply(const struct ipv4_header *ip, const struct icmp_echo *echo)
{
    uint8_t icmp[PAYLOAD_MAX];
    const uint8_t *mac = arp_lookup(ip->source);
    size_t length;

    if (mac == NULL || echo->payload_length > sizeof(icmp) - ICMP_HEADER_LEN)
        return;
    length = icmp_write_echo(icmp, ICMP_ECHO_REPLY, echo->id, echo->sequence, echo->payload, echo->payload_length);
    send_ip(mac, config.ip, ip->source, IP_PROTOCOL_ICMP, icmp, length);
}

static bool accepts_destination(const uint8_t destination[4])
{
    if (!config.configured)
        return true;
    return same_ip(destination, config.ip) || same_ip(destination, broadcast_ip);
}

static void handle_ipv4(const uint8_t *data, size_t length)
{
    struct ipv4_header ip;
    const uint8_t *payload;
    size_t payload_length;

    if (!ipv4_parse(data, length, &ip) || !accepts_destination(ip.destination))
        return;
    payload = data + ip.header_length;
    payload_length = ip.total_length - ip.header_length;

    if (ip.protocol == IP_PROTOCOL_ICMP) {
        struct icmp_echo echo;

        if (!icmp_parse_echo(payload, payload_length, &echo))
            return;
        if (echo.type == ICMP_ECHO_REQUEST && config.configured)
            send_ping_reply(&ip, &echo);
        else if (echo.type == ICMP_ECHO_REPLY && echo.id == PING_ID && echo.sequence == ping_expected_sequence)
            ping_reply_seen = true;
    } else if (ip.protocol == IP_PROTOCOL_UDP) {
        struct udp_datagram datagram;

        if (!udp_parse(payload, payload_length, ip.source, ip.destination, &datagram))
            return;
        for (size_t i = 0; i < UDP_HANDLERS; i++) {
            if (udp_handlers[i].handler != NULL && udp_handlers[i].port == datagram.destination_port) {
                udp_handlers[i].handler(ip.source, datagram.source_port, datagram.payload,
                                        datagram.payload_length);
                return;
            }
        }
    }
}

static void handle_arp(const uint8_t *data, size_t length)
{
    struct arp_packet packet;

    if (!arp_parse(data, length, &packet))
        return;
    if (config.configured && same_ip(packet.target_ip, config.ip)) {
        arp_remember(packet.sender_ip, packet.sender_mac);
        if (packet.operation == ARP_REQUEST)
            send_arp(ARP_REPLY, packet.sender_mac, packet.sender_ip);
    } else if (packet.operation == ARP_REPLY && arp_lookup(packet.sender_ip) == NULL) {
        arp_remember(packet.sender_ip, packet.sender_mac);
    }
}

void net_poll(void)
{
    if (!present)
        return;
    for (int i = 0; i < MAX_FRAMES_PER_POLL; i++) {
        int length = device.receive(rx_frame, sizeof(rx_frame));
        uint16_t type;

        if (length <= ETH_HEADER_LEN)
            return;
        type = get16(rx_frame + 12);
        if (type == ETH_TYPE_ARP)
            handle_arp(rx_frame + ETH_HEADER_LEN, (size_t)length - ETH_HEADER_LEN);
        else if (type == ETH_TYPE_IPV4)
            handle_ipv4(rx_frame + ETH_HEADER_LEN, (size_t)length - ETH_HEADER_LEN);
    }
}

static bool wait_for(const volatile bool *flag, unsigned timeout_ms)
{
    for (unsigned waited = 0; waited < timeout_ms; waited++) {
        net_poll();
        if (*flag)
            return true;
        timer_delay_ms(1);
    }
    net_poll();
    return *flag;
}

static bool resolve_mac(const uint8_t next_hop[4], uint8_t mac[6])
{
    for (int attempt = 0; attempt < ARP_RETRIES; attempt++) {
        const uint8_t *known = arp_lookup(next_hop);

        if (known != NULL) {
            memcpy(mac, known, 6);
            return true;
        }
        send_arp(ARP_REQUEST, broadcast_mac, next_hop);
        for (unsigned waited = 0; waited < ARP_RETRY_MS; waited++) {
            net_poll();
            if (arp_lookup(next_hop) != NULL)
                break;
            timer_delay_ms(1);
        }
    }
    {
        const uint8_t *known = arp_lookup(next_hop);

        if (known == NULL)
            return false;
        memcpy(mac, known, 6);
        return true;
    }
}

static bool send_udp(const uint8_t destination_ip[4], uint16_t source_port, uint16_t destination_port,
                     const uint8_t *payload, size_t payload_length)
{
    uint8_t segment[PAYLOAD_MAX];
    uint8_t mac[6];
    size_t length;

    if (payload_length > sizeof(segment) - UDP_HEADER_LEN)
        return false;

    if (same_ip(destination_ip, broadcast_ip)) {
        memcpy(mac, broadcast_mac, 6);
    } else if (!resolve_mac(on_link(destination_ip) || !config.has_gateway ? destination_ip : config.gateway, mac)) {
        return false;
    }
    length = udp_write(segment, config.ip, destination_ip, source_port, destination_port, payload, payload_length);
    return send_ip(mac, config.ip, destination_ip, IP_PROTOCOL_UDP, segment, length);
}

static void bind_udp(uint16_t port, udp_handler handler)
{
    for (size_t i = 0; i < UDP_HANDLERS; i++) {
        if (udp_handlers[i].handler == NULL || udp_handlers[i].port == port) {
            udp_handlers[i].port = port;
            udp_handlers[i].handler = handler;
            return;
        }
    }
}

static void unbind_udp(uint16_t port)
{
    for (size_t i = 0; i < UDP_HANDLERS; i++) {
        if (udp_handlers[i].handler != NULL && udp_handlers[i].port == port)
            udp_handlers[i].handler = NULL;
    }
}

bool net_init(void)
{
    if (!e1000_init(&device))
        return false;
    present = true;
    memset(&config, 0, sizeof(config));
    console_add_idle_hook(net_poll);
    return true;
}

bool net_available(void)
{
    return present;
}

const struct netdev *net_device(void)
{
    return present ? &device : NULL;
}

const struct net_config *net_get_config(void)
{
    return &config;
}

size_t net_arp_entry_count(void)
{
    return arp_count;
}

const struct arp_entry *net_arp_entry_at(size_t index)
{
    return index < arp_count ? &arp_cache[index] : NULL;
}

static void on_dhcp_reply(const uint8_t source_ip[4], uint16_t source_port, const uint8_t *payload, size_t length)
{
    struct dhcp_result result;

    (void)source_ip;
    (void)source_port;
    if (dhcp_parse(payload, length, dhcp_transaction, device.mac, &result)) {
        dhcp_reply = result;
        dhcp_reply_seen = true;
    }
}

/* Sends one DHCP message by broadcast before this host has an address. */
static void send_dhcp_message(const uint8_t *message, size_t length)
{
    uint8_t segment[PAYLOAD_MAX];
    size_t segment_length = udp_write(segment, zero_ip, broadcast_ip, DHCP_CLIENT_PORT, DHCP_SERVER_PORT, message,
                                      length);

    send_ip(broadcast_mac, zero_ip, broadcast_ip, IP_PROTOCOL_UDP, segment, segment_length);
}

static bool dhcp_exchange(const uint8_t *message, size_t length, uint8_t expected_type)
{
    for (int attempt = 0; attempt < DHCP_ATTEMPTS; attempt++) {
        dhcp_reply_seen = false;
        send_dhcp_message(message, length);
        if (wait_for(&dhcp_reply_seen, DHCP_STEP_MS) && dhcp_reply.type == expected_type)
            return true;
    }
    return false;
}

bool net_dhcp(void)
{
    uint8_t message[DHCP_MESSAGE_MIN + 64];
    struct dhcp_result offer;
    size_t length;

    if (!present)
        return false;

    memset(&config, 0, sizeof(config));
    dhcp_transaction = (uint32_t)timer_uptime_ms() ^ get32(device.mac + 2);
    bind_udp(DHCP_CLIENT_PORT, on_dhcp_reply);

    length = dhcp_write_discover(message, device.mac, dhcp_transaction);
    if (!dhcp_exchange(message, length, DHCP_OFFER)) {
        unbind_udp(DHCP_CLIENT_PORT);
        return false;
    }
    offer = dhcp_reply;
    if (!offer.has_server_id)
        memcpy(offer.server_id, zero_ip, 4);

    length = dhcp_write_request(message, device.mac, dhcp_transaction, offer.your_ip, offer.server_id);
    if (!dhcp_exchange(message, length, DHCP_ACK)) {
        unbind_udp(DHCP_CLIENT_PORT);
        return false;
    }
    unbind_udp(DHCP_CLIENT_PORT);

    memcpy(config.ip, dhcp_reply.your_ip, 4);
    if (dhcp_reply.has_mask)
        memcpy(config.mask, dhcp_reply.mask, 4);
    else
        memcpy(config.mask, (const uint8_t[4]){ 255, 255, 255, 0 }, 4);
    if (dhcp_reply.has_router) {
        memcpy(config.gateway, dhcp_reply.router, 4);
        config.has_gateway = true;
    }
    if (dhcp_reply.has_dns) {
        memcpy(config.dns, dhcp_reply.dns, 4);
        config.has_dns = true;
    }
    config.configured = true;
    return true;
}

static void on_dns_reply(const uint8_t source_ip[4], uint16_t source_port, const uint8_t *payload, size_t length)
{
    uint8_t address[4];

    (void)source_ip;
    (void)source_port;
    if (dns_parse_response(payload, length, dns_transaction, address)) {
        memcpy(dns_answer, address, 4);
        dns_reply_seen = true;
    }
}

bool net_resolve(const char *name, uint8_t address[4], unsigned timeout_ms)
{
    uint8_t query[300];
    size_t length;
    uint16_t port = (uint16_t)(EPHEMERAL_PORT_BASE + (timer_uptime_ms() & 0x3FFF));

    if (net_parse_ip(name, address))
        return true;
    if (!present || !config.configured || !config.has_dns)
        return false;

    dns_transaction = (uint16_t)(timer_uptime_ms() ^ port);
    length = dns_write_query(query, sizeof(query), dns_transaction, name);
    if (length == 0)
        return false;

    bind_udp(port, on_dns_reply);
    for (int attempt = 0; attempt < DNS_ATTEMPTS; attempt++) {
        dns_reply_seen = false;
        if (!send_udp(config.dns, port, DNS_PORT, query, length))
            continue;
        if (wait_for(&dns_reply_seen, timeout_ms / DNS_ATTEMPTS)) {
            memcpy(address, dns_answer, 4);
            unbind_udp(port);
            return true;
        }
    }
    unbind_udp(port);
    return false;
}

bool net_ping(const uint8_t address[4], unsigned timeout_ms, unsigned *round_trip_ms)
{
    static uint16_t sequence;
    uint8_t payload[PING_PAYLOAD];
    uint8_t icmp[ICMP_HEADER_LEN + PING_PAYLOAD];
    uint8_t mac[6];
    size_t length;
    uint64_t sent;

    if (!present || !config.configured)
        return false;
    if (!resolve_mac(on_link(address) || !config.has_gateway ? address : config.gateway, mac))
        return false;

    for (size_t i = 0; i < sizeof(payload); i++)
        payload[i] = (uint8_t)('a' + i % 26);
    ping_expected_sequence = ++sequence;
    ping_reply_seen = false;
    length = icmp_write_echo(icmp, ICMP_ECHO_REQUEST, PING_ID, ping_expected_sequence, payload, sizeof(payload));

    sent = timer_uptime_ms();
    if (!send_ip(mac, config.ip, address, IP_PROTOCOL_ICMP, icmp, length))
        return false;
    if (!wait_for(&ping_reply_seen, timeout_ms))
        return false;
    if (round_trip_ms != NULL)
        *round_trip_ms = (unsigned)(timer_uptime_ms() - sent);
    return true;
}
