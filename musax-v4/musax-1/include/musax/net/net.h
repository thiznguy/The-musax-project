#ifndef MUSAX_NET_NET_H
#define MUSAX_NET_NET_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define NET_ARP_CACHE_SIZE 8

/* A network adapter as the stack sees it; drivers fill this in. receive returns the frame length, or 0 when none waits. */
struct netdev {
    char name[8];
    uint8_t mac[6];
    int (*send)(const void *frame, size_t length);
    int (*receive)(void *buffer, size_t capacity);
    bool (*link_up)(void);
};

struct net_config {
    bool configured;
    uint8_t ip[4];
    uint8_t mask[4];
    uint8_t gateway[4];
    uint8_t dns[4];
    bool has_gateway;
    bool has_dns;
};

struct arp_entry {
    uint8_t ip[4];
    uint8_t mac[6];
};

/* Finds and starts a network adapter. Returns false when the machine has none. */
bool net_init(void);
bool net_available(void);
const struct netdev *net_device(void);
const struct net_config *net_get_config(void);

/* Drains received frames and answers ARP and ping requests; call it regularly. */
void net_poll(void);

/* Obtains an address, gateway and DNS server from a DHCP server; gives up after a few seconds. */
bool net_dhcp(void);

/* Turns a dotted address or a host name into an address. */
bool net_resolve(const char *name, uint8_t address[4], unsigned timeout_ms);

/* One echo request; returns true and the round trip time in milliseconds when the reply arrived. */
bool net_ping(const uint8_t address[4], unsigned timeout_ms, unsigned *round_trip_ms);

size_t net_arp_entry_count(void);
const struct arp_entry *net_arp_entry_at(size_t index);

bool net_parse_ip(const char *text, uint8_t address[4]);
void net_format_ip(const uint8_t address[4], char *out, size_t capacity);
void net_format_mac(const uint8_t mac[6], char *out, size_t capacity);

#endif
