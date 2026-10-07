#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <musax/drivers/pci.h>
#include <musax/drivers/timer.h>
#include <musax/kprintf.h>
#include <musax/net/net.h>
#include <musax/gui/desktop.h>
#include "../internal.h"

#define PING_COUNT_DEFAULT 4
#define PING_COUNT_MAX 100
#define PING_TIMEOUT_MS 2000
#define PING_INTERVAL_MS 1000
#define RESOLVE_TIMEOUT_MS 4000

static bool require_adapter(const char *command)
{
    if (net_available())
        return true;
    kprintf("%s: no network adapter found (start QEMU with -nic user,model=e1000)\n", command);
    return false;
}

static bool require_address(const char *command)
{
    if (!require_adapter(command))
        return false;
    if (net_get_config()->configured)
        return true;
    kprintf("%s: no IP address yet, run dhcp first\n", command);
    return false;
}

int command_ifconfig(int argc, char **argv)
{
    const struct netdev *device = net_device();
    const struct net_config *config = net_get_config();
    char text[24];

    (void)argc;
    (void)argv;

    if (!require_adapter("ifconfig"))
        return 1;

    net_format_mac(device->mac, text, sizeof(text));
    kprintf("%s: link %s, mac %s\n", device->name, device->link_up() ? "up" : "down", text);
    if (!config->configured) {
        kprintf("  no IP address (run dhcp)\n");
        return 0;
    }
    net_format_ip(config->ip, text, sizeof(text));
    kprintf("  address %s", text);
    net_format_ip(config->mask, text, sizeof(text));
    kprintf("  netmask %s\n", text);
    if (config->has_gateway) {
        net_format_ip(config->gateway, text, sizeof(text));
        kprintf("  gateway %s\n", text);
    }
    if (config->has_dns) {
        net_format_ip(config->dns, text, sizeof(text));
        kprintf("  dns     %s\n", text);
    }
    return 0;
}

int command_dhcp(int argc, char **argv)
{
    const struct net_config *config = net_get_config();
    char text[24];

    (void)argc;
    (void)argv;

    if (!require_adapter("dhcp"))
        return 1;
    kprintf("requesting an address...\n");
    if (!net_dhcp()) {
        kprintf("dhcp: no answer from a DHCP server\n");
        return 1;
    }
    net_format_ip(config->ip, text, sizeof(text));
    kprintf("address %s\n", text);
    return 0;
}

int command_ping(int argc, char **argv)
{
    uint8_t address[4];
    char text[24];
    long count = PING_COUNT_DEFAULT;
    unsigned received = 0;

    if (argc < 2 || argc > 3) {
        kprintf("usage: ping host [count]\n");
        return 1;
    }
    if (argc == 3) {
        char *end;

        count = strtol(argv[2], &end, 10);
        if (*end != '\0' || count < 1 || count > PING_COUNT_MAX) {
            kprintf("ping: count must be from 1 to %d\n", PING_COUNT_MAX);
            return 1;
        }
    }
    if (!require_address("ping"))
        return 1;
    if (!net_resolve(argv[1], address, RESOLVE_TIMEOUT_MS)) {
        kprintf("ping: cannot resolve %s\n", argv[1]);
        return 1;
    }

    net_format_ip(address, text, sizeof(text));
    kprintf("ping %s (%s)\n", argv[1], text);
    for (long i = 1; i <= count; i++) {
        unsigned round_trip;

        if (net_ping(address, PING_TIMEOUT_MS, &round_trip)) {
            received++;
            kprintf("reply from %s: seq=%ld time=%u ms\n", text, i, round_trip);
        } else {
            kprintf("seq=%ld: no reply\n", i);
        }
        if (i < count)
            timer_delay_ms(PING_INTERVAL_MS);
    }
    kprintf("%ld sent, %u received\n", count, received);
    return received > 0 ? 0 : 1;
}

int command_nslookup(int argc, char **argv)
{
    uint8_t address[4];
    char text[24];

    if (argc != 2) {
        kprintf("usage: nslookup name\n");
        return 1;
    }
    if (!require_address("nslookup"))
        return 1;
    if (!net_resolve(argv[1], address, RESOLVE_TIMEOUT_MS)) {
        kprintf("nslookup: cannot resolve %s\n", argv[1]);
        return 1;
    }
    net_format_ip(address, text, sizeof(text));
    kprintf("%s has address %s\n", argv[1], text);
    return 0;
}

int command_arp(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    if (!require_adapter("arp"))
        return 1;
    if (net_arp_entry_count() == 0)
        kprintf("the ARP cache is empty\n");
    for (size_t i = 0; i < net_arp_entry_count(); i++) {
        const struct arp_entry *entry = net_arp_entry_at(i);
        char ip[24];
        char mac[24];

        net_format_ip(entry->ip, ip, sizeof(ip));
        net_format_mac(entry->mac, mac, sizeof(mac));
        kprintf("%-16s %s\n", ip, mac);
    }
    return 0;
}

int command_lspci(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    for (size_t i = 0; i < pci_device_count(); i++) {
        const struct pci_device *device = pci_device_at(i);
        const char *name = pci_device_name(device->vendor_id, device->device_id);

        kprintf("%02x:%02x.%u %04x:%04x %s%s%s\n", device->bus, device->slot, device->function, device->vendor_id,
                device->device_id, pci_class_name(device->class_code, device->subclass), name != NULL ? ": " : "",
                name != NULL ? name : "");
    }
    return 0;
}

int command_desktop(int argc, char **argv)
{
    int error;

    (void)argc;
    (void)argv;

    error = desktop_run();
    if (error != DESKTOP_OK) {
        kprintf("desktop: %s\n", desktop_error_message(error));
        return 1;
    }
    return 0;
}
