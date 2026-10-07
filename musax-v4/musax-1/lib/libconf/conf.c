#include <ctype.h>
#include <stddef.h>
#include <string.h>
#include <libconf/conf.h>

#define CONF_LINE_MAX 256
#define CONF_SECTION_MAX 32

static char *trim(char *text)
{
    char *end;

    while (isspace((unsigned char)*text))
        text++;
    end = text + strlen(text);
    while (end > text && isspace((unsigned char)end[-1]))
        end--;
    *end = '\0';
    return text;
}

static char *unquote(char *value)
{
    size_t length = strlen(value);

    if (length >= 2 && value[0] == '"' && value[length - 1] == '"') {
        value[length - 1] = '\0';
        return value + 1;
    }
    return value;
}

int conf_parse(const char *text, size_t length, conf_handler handler, void *context)
{
    char section[CONF_SECTION_MAX] = "";
    size_t position = 0;
    int delivered = 0;

    while (position < length) {
        char line[CONF_LINE_MAX];
        char key[CONF_KEY_MAX];
        size_t line_length = 0;
        char *content;
        char *separator;
        char *name;
        char *value;
        size_t section_length;
        size_t name_length;

        while (position < length && text[position] != '\n') {
            if (line_length + 1 < sizeof(line))
                line[line_length++] = text[position];
            position++;
        }
        if (position < length)
            position++;
        line[line_length] = '\0';

        content = trim(line);
        if (content[0] == '\0' || content[0] == '#' || content[0] == ';')
            continue;

        if (content[0] == '[') {
            char *close = strchr(content, ']');

            if (close != NULL && (size_t)(close - content) - 1 < sizeof(section)) {
                *close = '\0';
                strcpy(section, trim(content + 1));
            }
            continue;
        }

        separator = strchr(content, '=');
        if (separator == NULL)
            continue;
        *separator = '\0';
        name = trim(content);
        value = unquote(trim(separator + 1));

        section_length = strlen(section);
        name_length = strlen(name);
        if (name_length == 0 || section_length + 1 + name_length + 1 > sizeof(key))
            continue;

        key[0] = '\0';
        if (section_length > 0) {
            strcpy(key, section);
            key[section_length] = '.';
            key[section_length + 1] = '\0';
        }
        strcat(key, name);

        if (handler(context, key, value) != 0)
            return -1;
        delivered++;
    }

    return delivered;
}
