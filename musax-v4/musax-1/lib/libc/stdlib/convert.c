#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>

static int digit_value(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'z')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'Z')
        return c - 'A' + 10;
    return 99;
}

static unsigned long parse_magnitude(const char *string, char **end, int base, bool *negative, bool *overflow)
{
    const char *cursor = string;
    unsigned long value = 0;
    bool any_digit = false;

    *negative = false;
    *overflow = false;

    while (isspace((unsigned char)*cursor))
        cursor++;
    if (*cursor == '+' || *cursor == '-') {
        *negative = *cursor == '-';
        cursor++;
    }
    if ((base == 0 || base == 16) && cursor[0] == '0' && (cursor[1] == 'x' || cursor[1] == 'X') &&
        digit_value(cursor[2]) < 16) {
        cursor += 2;
        base = 16;
    } else if (base == 0) {
        base = cursor[0] == '0' ? 8 : 10;
    }

    for (;; cursor++) {
        int digit = digit_value(*cursor);

        if (digit >= base)
            break;
        any_digit = true;
        if (value > (ULONG_MAX - (unsigned long)digit) / (unsigned long)base)
            *overflow = true;
        else
            value = value * (unsigned long)base + (unsigned long)digit;
    }

    if (end != NULL)
        *end = (char *)(any_digit ? cursor : string);
    return value;
}

unsigned long strtoul(const char *string, char **end, int base)
{
    bool negative;
    bool overflow;
    unsigned long value = parse_magnitude(string, end, base, &negative, &overflow);

    if (overflow)
        return ULONG_MAX;
    return negative ? 0UL - value : value;
}

long strtol(const char *string, char **end, int base)
{
    bool negative;
    bool overflow;
    unsigned long value = parse_magnitude(string, end, base, &negative, &overflow);

    if (negative) {
        if (overflow || value > (unsigned long)LONG_MAX + 1UL)
            return LONG_MIN;
        return (long)(0UL - value);
    }
    if (overflow || value > (unsigned long)LONG_MAX)
        return LONG_MAX;
    return (long)value;
}

int atoi(const char *string)
{
    return (int)strtol(string, NULL, 10);
}

int abs(int value)
{
    return value < 0 ? -value : value;
}

long labs(long value)
{
    return value < 0 ? -value : value;
}
