#include <string.h>
#include <libconf/conf.h>
#include "check.h"

struct pairs {
    char text[512];
    int count;
};

static int collect(void *context, const char *key, const char *value)
{
    struct pairs *pairs = context;

    strcat(pairs->text, key);
    strcat(pairs->text, "=");
    strcat(pairs->text, value);
    strcat(pairs->text, ";");
    pairs->count++;
    return 0;
}

static int stop_at_second(void *context, const char *key, const char *value)
{
    int *seen = context;

    (void)key;
    (void)value;
    return ++(*seen) == 2;
}

static const char *parse(const char *text)
{
    static struct pairs pairs;

    pairs.text[0] = '\0';
    pairs.count = 0;
    conf_parse(text, strlen(text), collect, &pairs);
    return pairs.text;
}

int main(void)
{
    int seen = 0;
    const char *two = "a = 1\nb = 2\nc = 3\n";

    CHECK_STR(parse(""), "");
    CHECK_STR(parse("# comment\n; other\n\n   \n"), "");
    CHECK_STR(parse("name = value"), "name=value;");
    CHECK_STR(parse("  spaced   =   padded value  \n"), "spaced=padded value;");
    CHECK_STR(parse("[system]\nhostname = musax\n[boot]\nanimation = on\n"), "system.hostname=musax;boot.animation=on;");
    CHECK_STR(parse("top = 1\n[s]\nin = 2\n"), "top=1;s.in=2;");
    CHECK_STR(parse("quoted = \"  keep spaces  \"\n"), "quoted=  keep spaces  ;");
    CHECK_STR(parse("empty =\n"), "empty=;");
    CHECK_STR(parse("no separator line\nok = 1\n"), "ok=1;");
    CHECK_STR(parse("= missing key\nok = 1\n"), "ok=1;");
    CHECK_STR(parse("a = 1\r\nb = 2\r\n"), "a=1;b=2;");
    CHECK_STR(parse("url = a=b=c\n"), "url=a=b=c;");
    CHECK_STR(parse("[sec]\n[other]\nk = v\n"), "other.k=v;");

    CHECK(conf_parse(two, strlen(two), stop_at_second, &seen) == -1);
    CHECK(seen == 2);
    CHECK(conf_parse(two, strlen(two), collect, &(struct pairs){ .text = "" }) == 3);

    CHECK_REPORT("conf");
}
