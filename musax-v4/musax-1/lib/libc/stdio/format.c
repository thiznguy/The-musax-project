#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define NUMBER_BUFFER_SIZE 24

/* Parsed flags of one conversion: left alignment, zero padding, minimum width, and size modifier (0 int, 1 long, 2 long long). */
struct format_spec {
    bool left_align;
    bool zero_pad;
    int width;
    int length;
};

/* Output callback plus the running count of emitted characters. */
struct output {
    void (*emit)(char c, void *context);
    void *context;
    int count;
};

static void emit_char(struct output *out, char c)
{
    out->emit(c, out->context);
    out->count++;
}

static void emit_repeat(struct output *out, char c, int count)
{
    while (count-- > 0)
        emit_char(out, c);
}

static void emit_field(struct output *out, const char *body, size_t length, bool negative,
                       const struct format_spec *spec)
{
    int content = (int)length + (negative ? 1 : 0);
    int padding = spec->width > content ? spec->width - content : 0;

    if (spec->left_align) {
        if (negative)
            emit_char(out, '-');
        for (size_t i = 0; i < length; i++)
            emit_char(out, body[i]);
        emit_repeat(out, ' ', padding);
        return;
    }

    if (!spec->zero_pad)
        emit_repeat(out, ' ', padding);
    if (negative)
        emit_char(out, '-');
    if (spec->zero_pad)
        emit_repeat(out, '0', padding);
    for (size_t i = 0; i < length; i++)
        emit_char(out, body[i]);
}

static size_t format_unsigned(char *buffer, unsigned long long value, unsigned base, bool upper)
{
    static const char lower_digits[] = "0123456789abcdef";
    static const char upper_digits[] = "0123456789ABCDEF";
    const char *digits = upper ? upper_digits : lower_digits;
    char reversed[NUMBER_BUFFER_SIZE];
    size_t count = 0;

    do {
        reversed[count++] = digits[value % base];
        value /= base;
    } while (value != 0);

    for (size_t i = 0; i < count; i++)
        buffer[i] = reversed[count - 1 - i];
    return count;
}

int __vformat(void (*emit)(char c, void *context), void *context, const char *format, va_list args)
{
    struct output out = { .emit = emit, .context = context, .count = 0 };
    char digits[NUMBER_BUFFER_SIZE];

    for (; *format != '\0'; format++) {
        struct format_spec spec = { .left_align = false, .zero_pad = false, .width = 0, .length = 0 };

        if (*format != '%') {
            emit_char(&out, *format);
            continue;
        }

        format++;
        if (*format == '-') {
            spec.left_align = true;
            format++;
        }
        if (*format == '0') {
            spec.zero_pad = true;
            format++;
        }
        while (*format >= '0' && *format <= '9') {
            spec.width = spec.width * 10 + (*format - '0');
            format++;
        }
        while (*format == 'l') {
            spec.length++;
            format++;
        }
        if (*format == 'z') {
            spec.length = 1;
            format++;
        }

        switch (*format) {
        case '\0':
            return out.count;
        case 'c': {
            char c = (char)va_arg(args, int);

            emit_field(&out, &c, 1, false, &spec);
            break;
        }
        case 's': {
            const char *string = va_arg(args, const char *);

            if (string == NULL)
                string = "(null)";
            emit_field(&out, string, strlen(string), false, &spec);
            break;
        }
        case 'd':
        case 'i': {
            long long value;
            bool negative;
            unsigned long long magnitude;

            if (spec.length >= 2)
                value = va_arg(args, long long);
            else if (spec.length == 1)
                value = va_arg(args, long);
            else
                value = va_arg(args, int);

            negative = value < 0;
            magnitude = negative ? 0ULL - (unsigned long long)value : (unsigned long long)value;
            emit_field(&out, digits, format_unsigned(digits, magnitude, 10, false), negative, &spec);
            break;
        }
        case 'u':
        case 'x':
        case 'X': {
            unsigned long long value;
            unsigned base = *format == 'u' ? 10 : 16;

            if (spec.length >= 2)
                value = va_arg(args, unsigned long long);
            else if (spec.length == 1)
                value = va_arg(args, unsigned long);
            else
                value = va_arg(args, unsigned int);

            emit_field(&out, digits, format_unsigned(digits, value, base, *format == 'X'), false, &spec);
            break;
        }
        case 'p': {
            struct format_spec pointer_spec = { .left_align = false, .zero_pad = true, .width = 16, .length = 0 };
            uintptr_t value = (uintptr_t)va_arg(args, void *);

            emit_char(&out, '0');
            emit_char(&out, 'x');
            emit_field(&out, digits, format_unsigned(digits, value, 16, false), false, &pointer_spec);
            break;
        }
        case '%':
            emit_char(&out, '%');
            break;
        default:
            emit_char(&out, '%');
            emit_char(&out, *format);
            break;
        }
    }

    return out.count;
}
