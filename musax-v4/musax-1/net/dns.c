#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <musax/net/endian.h>
#include <musax/net/proto.h>

#define DNS_HEADER_LEN 12
#define DNS_FLAG_RECURSION_DESIRED 0x0100
#define DNS_FLAG_RESPONSE 0x8000
#define DNS_RCODE_MASK 0x000F
#define DNS_TYPE_A 1
#define DNS_CLASS_IN 1
#define DNS_LABEL_MAX 63
#define DNS_POINTER_MASK 0xC0
#define DNS_MAX_ANSWERS 32

size_t dns_write_query(uint8_t *out, size_t capacity, uint16_t id, const char *name)
{
    size_t position = DNS_HEADER_LEN;
    const char *label = name;

    if (name[0] == '\0' || capacity < DNS_HEADER_LEN + strlen(name) + 2 + 4)
        return 0;

    put16(out, id);
    put16(out + 2, DNS_FLAG_RECURSION_DESIRED);
    put16(out + 4, 1);
    put16(out + 6, 0);
    put16(out + 8, 0);
    put16(out + 10, 0);

    for (;;) {
        const char *end = strchr(label, '.');
        size_t length = end != NULL ? (size_t)(end - label) : strlen(label);

        if (length == 0 || length > DNS_LABEL_MAX)
            return 0;
        out[position++] = (uint8_t)length;
        memcpy(out + position, label, length);
        position += length;
        if (end == NULL)
            break;
        label = end + 1;
    }
    out[position++] = 0;
    put16(out + position, DNS_TYPE_A);
    put16(out + position + 2, DNS_CLASS_IN);
    return position + 4;
}

/* Position just after the name that starts at position, or 0 when the name is malformed. */
static size_t skip_name(const uint8_t *data, size_t length, size_t position)
{
    while (position < length) {
        uint8_t label = data[position];

        if (label == 0)
            return position + 1;
        if ((label & DNS_POINTER_MASK) == DNS_POINTER_MASK)
            return position + 2 <= length ? position + 2 : 0;
        if (label & DNS_POINTER_MASK)
            return 0;
        position += 1 + (size_t)label;
    }
    return 0;
}

bool dns_parse_response(const uint8_t *data, size_t length, uint16_t id, uint8_t address[4])
{
    size_t position = DNS_HEADER_LEN;
    unsigned questions;
    unsigned answers;

    if (length < DNS_HEADER_LEN || get16(data) != id)
        return false;
    if (!(get16(data + 2) & DNS_FLAG_RESPONSE) || (get16(data + 2) & DNS_RCODE_MASK) != 0)
        return false;

    questions = get16(data + 4);
    answers = get16(data + 6);
    if (answers > DNS_MAX_ANSWERS)
        answers = DNS_MAX_ANSWERS;

    for (unsigned i = 0; i < questions; i++) {
        position = skip_name(data, length, position);
        if (position == 0 || position + 4 > length)
            return false;
        position += 4;
    }

    for (unsigned i = 0; i < answers; i++) {
        uint16_t type;
        uint16_t class_code;
        uint16_t data_length;

        position = skip_name(data, length, position);
        if (position == 0 || position + 10 > length)
            return false;
        type = get16(data + position);
        class_code = get16(data + position + 2);
        data_length = get16(data + position + 8);
        position += 10;
        if (position + data_length > length)
            return false;
        if (type == DNS_TYPE_A && class_code == DNS_CLASS_IN && data_length == 4) {
            memcpy(address, data + position, 4);
            return true;
        }
        position += data_length;
    }
    return false;
}
