#ifndef MUSAX_NET_ENDIAN_H
#define MUSAX_NET_ENDIAN_H

#include <stdint.h>

static inline uint16_t net_htons(uint16_t value)
{
    return (uint16_t)((value << 8) | (value >> 8));
}

static inline uint16_t net_ntohs(uint16_t value)
{
    return net_htons(value);
}

static inline uint32_t net_htonl(uint32_t value)
{
    return ((value & 0xFFU) << 24) | ((value & 0xFF00U) << 8) | ((value >> 8) & 0xFF00U) | (value >> 24);
}

static inline uint32_t net_ntohl(uint32_t value)
{
    return net_htonl(value);
}

static inline void put16(uint8_t *out, uint16_t value)
{
    out[0] = (uint8_t)(value >> 8);
    out[1] = (uint8_t)value;
}

static inline void put32(uint8_t *out, uint32_t value)
{
    out[0] = (uint8_t)(value >> 24);
    out[1] = (uint8_t)(value >> 16);
    out[2] = (uint8_t)(value >> 8);
    out[3] = (uint8_t)value;
}

static inline uint16_t get16(const uint8_t *in)
{
    return (uint16_t)((in[0] << 8) | in[1]);
}

static inline uint32_t get32(const uint8_t *in)
{
    return ((uint32_t)in[0] << 24) | ((uint32_t)in[1] << 16) | ((uint32_t)in[2] << 8) | in[3];
}

#endif
