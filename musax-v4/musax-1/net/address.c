#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <musax/net/net.h>

#define IPV4_OCTETS 4
#define OCTET_MAX 255

bool net_parse_ip(const char *text, uint8_t address[4])
{
    uint8_t parsed[IPV4_OCTETS];

    for (int octet = 0; octet < IPV4_OCTETS; octet++) {
        unsigned value = 0;
        int digits = 0;

        while (*text >= '0' && *text <= '9') {
            value = value * 10 + (unsigned)(*text - '0');
            if (value > OCTET_MAX)
                return false;
            digits++;
            text++;
        }
        if (digits == 0 || digits > 3)
            return false;
        parsed[octet] = (uint8_t)value;
        if (octet < IPV4_OCTETS - 1) {
            if (*text != '.')
                return false;
            text++;
        }
    }
    if (*text != '\0')
        return false;
    for (int i = 0; i < IPV4_OCTETS; i++)
        address[i] = parsed[i];
    return true;
}

void net_format_ip(const uint8_t address[4], char *out, size_t capacity)
{
    snprintf(out, capacity, "%u.%u.%u.%u", address[0], address[1], address[2], address[3]);
}

void net_format_mac(const uint8_t mac[6], char *out, size_t capacity)
{
    snprintf(out, capacity, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}
