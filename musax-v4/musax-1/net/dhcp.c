#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <musax/net/endian.h>
#include <musax/net/proto.h>

#define BOOTP_REQUEST 1
#define BOOTP_REPLY 2
#define BOOTP_HARDWARE_ETHERNET 1
#define BOOTP_FLAG_BROADCAST 0x8000
#define BOOTP_FIXED_LEN 236
#define DHCP_MAGIC 0x63825363U

#define OPTION_PAD 0
#define OPTION_MASK 1
#define OPTION_ROUTER 3
#define OPTION_DNS 6
#define OPTION_REQUESTED_IP 50
#define OPTION_LEASE_TIME 51
#define OPTION_MESSAGE_TYPE 53
#define OPTION_SERVER_ID 54
#define OPTION_PARAMETER_LIST 55
#define OPTION_END 255

static size_t write_header(uint8_t *out, const uint8_t mac[6], uint32_t transaction_id)
{
    memset(out, 0, DHCP_MESSAGE_MIN);
    out[0] = BOOTP_REQUEST;
    out[1] = BOOTP_HARDWARE_ETHERNET;
    out[2] = 6;
    put32(out + 4, transaction_id);
    put16(out + 10, BOOTP_FLAG_BROADCAST);
    memcpy(out + 28, mac, 6);
    put32(out + BOOTP_FIXED_LEN, DHCP_MAGIC);
    return BOOTP_FIXED_LEN + 4;
}

static size_t write_option(uint8_t *out, size_t position, uint8_t code, const void *value, uint8_t length)
{
    out[position++] = code;
    out[position++] = length;
    memcpy(out + position, value, length);
    return position + length;
}

static size_t finish(uint8_t *out, size_t position)
{
    out[position++] = OPTION_END;
    return position < DHCP_MESSAGE_MIN ? DHCP_MESSAGE_MIN : position;
}

size_t dhcp_write_discover(uint8_t *out, const uint8_t mac[6], uint32_t transaction_id)
{
    static const uint8_t wanted[] = { OPTION_MASK, OPTION_ROUTER, OPTION_DNS, OPTION_LEASE_TIME };
    size_t position = write_header(out, mac, transaction_id);
    uint8_t type = DHCP_DISCOVER;

    position = write_option(out, position, OPTION_MESSAGE_TYPE, &type, 1);
    position = write_option(out, position, OPTION_PARAMETER_LIST, wanted, sizeof(wanted));
    return finish(out, position);
}

size_t dhcp_write_request(uint8_t *out, const uint8_t mac[6], uint32_t transaction_id, const uint8_t requested_ip[4],
                          const uint8_t server_id[4])
{
    static const uint8_t wanted[] = { OPTION_MASK, OPTION_ROUTER, OPTION_DNS, OPTION_LEASE_TIME };
    size_t position = write_header(out, mac, transaction_id);
    uint8_t type = DHCP_REQUEST;

    position = write_option(out, position, OPTION_MESSAGE_TYPE, &type, 1);
    position = write_option(out, position, OPTION_REQUESTED_IP, requested_ip, 4);
    position = write_option(out, position, OPTION_SERVER_ID, server_id, 4);
    position = write_option(out, position, OPTION_PARAMETER_LIST, wanted, sizeof(wanted));
    return finish(out, position);
}

bool dhcp_parse(const uint8_t *data, size_t length, uint32_t transaction_id, const uint8_t mac[6],
                struct dhcp_result *result)
{
    size_t position = BOOTP_FIXED_LEN + 4;

    if (length < position || data[0] != BOOTP_REPLY || get32(data + 4) != transaction_id ||
        get32(data + BOOTP_FIXED_LEN) != DHCP_MAGIC || memcmp(data + 28, mac, 6) != 0)
        return false;

    memset(result, 0, sizeof(*result));
    memcpy(result->your_ip, data + 16, 4);

    while (position < length) {
        uint8_t code = data[position++];
        uint8_t option_length;

        if (code == OPTION_PAD)
            continue;
        if (code == OPTION_END)
            break;
        if (position >= length)
            return false;
        option_length = data[position++];
        if (position + option_length > length)
            return false;

        switch (code) {
        case OPTION_MESSAGE_TYPE:
            if (option_length == 1)
                result->type = data[position];
            break;
        case OPTION_SERVER_ID:
            if (option_length == 4) {
                memcpy(result->server_id, data + position, 4);
                result->has_server_id = true;
            }
            break;
        case OPTION_MASK:
            if (option_length == 4) {
                memcpy(result->mask, data + position, 4);
                result->has_mask = true;
            }
            break;
        case OPTION_ROUTER:
            if (option_length >= 4) {
                memcpy(result->router, data + position, 4);
                result->has_router = true;
            }
            break;
        case OPTION_DNS:
            if (option_length >= 4) {
                memcpy(result->dns, data + position, 4);
                result->has_dns = true;
            }
            break;
        case OPTION_LEASE_TIME:
            if (option_length == 4)
                result->lease_seconds = get32(data + position);
            break;
        default:
            break;
        }
        position += option_length;
    }
    return result->type != 0;
}
