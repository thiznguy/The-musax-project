#include <ctype.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "check.h"

/* The libc under test is linked with every symbol renamed mx_<name>, so it cannot collide with the host libc. */
struct mx_tm {
    int tm_sec, tm_min, tm_hour, tm_mday, tm_mon, tm_year, tm_wday, tm_yday, tm_isdst;
};

struct mx_malloc_info {
    size_t arena_bytes, used_bytes, free_bytes, largest_free_block, block_count;
};

void *mx_memcpy(void *, const void *, size_t);
void *mx_memmove(void *, const void *, size_t);
void *mx_memset(void *, int, size_t);
int mx_memcmp(const void *, const void *, size_t);
void *mx_memchr(const void *, int, size_t);
size_t mx_strlen(const char *);
size_t mx_strnlen(const char *, size_t);
int mx_strcmp(const char *, const char *);
int mx_strncmp(const char *, const char *, size_t);
char *mx_strcpy(char *, const char *);
char *mx_strncpy(char *, const char *, size_t);
size_t mx_strlcpy(char *, const char *, size_t);
char *mx_strcat(char *, const char *);
char *mx_strchr(const char *, int);
char *mx_strrchr(const char *, int);
char *mx_strstr(const char *, const char *);
int mx_isalnum(int), mx_isalpha(int), mx_isdigit(int), mx_isxdigit(int), mx_islower(int), mx_isupper(int);
int mx_isspace(int), mx_isprint(int), mx_ispunct(int), mx_tolower(int), mx_toupper(int);
long mx_strtol(const char *, char **, int);
unsigned long mx_strtoul(const char *, char **, int);
int mx_atoi(const char *);
int mx_abs(int);
long mx_labs(long);
int mx_snprintf(char *, size_t, const char *, ...);
void *mx_malloc(size_t);
void *mx_calloc(size_t, size_t);
void *mx_realloc(void *, size_t);
void mx_free(void *);
void mx___malloc_init(void *, size_t);
void mx___malloc_get_info(struct mx_malloc_info *);
long long mx_timegm(struct mx_tm *);
struct mx_tm *mx_gmtime_r(const long long *, struct mx_tm *);

static int sign(int value)
{
    return (value > 0) - (value < 0);
}

static void test_string(void)
{
    char a[32];
    char b[32];
    const char *text = "hello, world";

    CHECK(mx_strlen("") == 0);
    CHECK(mx_strlen(text) == 12);
    CHECK(mx_strnlen(text, 5) == 5);
    CHECK(mx_strnlen(text, 100) == 12);

    CHECK(sign(mx_strcmp("a", "b")) == sign(strcmp("a", "b")));
    CHECK(sign(mx_strcmp("b", "a")) == sign(strcmp("b", "a")));
    CHECK(mx_strcmp("same", "same") == 0);
    CHECK(sign(mx_strcmp("abc", "abcd")) == sign(strcmp("abc", "abcd")));
    CHECK(sign(mx_strcmp("\xff", "a")) == sign(strcmp("\xff", "a")));
    CHECK(mx_strncmp("abcX", "abcY", 3) == 0);
    CHECK(sign(mx_strncmp("abcX", "abcY", 4)) == sign(strncmp("abcX", "abcY", 4)));
    CHECK(mx_strncmp("a", "b", 0) == 0);

    CHECK(mx_strchr(text, 'w') == text + 7);
    CHECK(mx_strchr(text, 'z') == NULL);
    CHECK(mx_strchr(text, '\0') == text + 12);
    CHECK(mx_strrchr(text, 'o') == text + 8);
    CHECK(mx_strrchr(text, 'z') == NULL);
    CHECK(mx_strstr(text, "world") == text + 7);
    CHECK(mx_strstr(text, "") == text);
    CHECK(mx_strstr(text, "worlds") == NULL);
    CHECK(mx_memchr(text, 'w', 12) == text + 7);
    CHECK(mx_memchr(text, 'w', 7) == NULL);

    mx_strcpy(a, "abc");
    mx_strcat(a, "def");
    CHECK_STR(a, "abcdef");

    memset(b, 'x', sizeof(b));
    mx_strncpy(b, "ab", 5);
    CHECK(memcmp(b, "ab\0\0\0x", 6) == 0);

    CHECK(mx_strlcpy(a, "truncate me", 5) == 11);
    CHECK_STR(a, "trun");
    CHECK(mx_strlcpy(a, "ok", 32) == 2);
    CHECK_STR(a, "ok");
    CHECK(mx_strlcpy(a, "zero", 0) == 4);
    CHECK_STR(a, "ok");

    mx_memset(a, 'q', 8);
    CHECK(memcmp(a, "qqqqqqqq", 8) == 0);
    mx_memcpy(b, "0123456789", 11);
    CHECK_STR(b, "0123456789");
    mx_memmove(b + 2, b, 8);
    CHECK(memcmp(b, "0101234567", 10) == 0);
    mx_memmove(b, b + 2, 8);
    CHECK(memcmp(b, "0123456767", 10) == 0);
    CHECK(mx_memcmp("abc", "abd", 3) < 0);
    CHECK(mx_memcmp("abc", "abd", 2) == 0);
}

static void test_ctype(void)
{
    for (int c = 0; c < 128; c++) {
        CHECK((mx_isalnum(c) != 0) == (isalnum(c) != 0));
        CHECK((mx_isalpha(c) != 0) == (isalpha(c) != 0));
        CHECK((mx_isdigit(c) != 0) == (isdigit(c) != 0));
        CHECK((mx_isxdigit(c) != 0) == (isxdigit(c) != 0));
        CHECK((mx_islower(c) != 0) == (islower(c) != 0));
        CHECK((mx_isupper(c) != 0) == (isupper(c) != 0));
        CHECK((mx_isspace(c) != 0) == (isspace(c) != 0));
        CHECK((mx_isprint(c) != 0) == (isprint(c) != 0));
        CHECK((mx_ispunct(c) != 0) == (ispunct(c) != 0));
        CHECK(mx_tolower(c) == tolower(c));
        CHECK(mx_toupper(c) == toupper(c));
    }
}

static void test_convert(void)
{
    static const char *const samples[] = {
        "0", "42", "-42", "+7", "  12abc", "abc", "", "0x1f", "0X1F", "010", "9223372036854775807",
        "9223372036854775808", "-9223372036854775808", "-9223372036854775809", "99999999999999999999", "0x", "0xg",
        "  -0", "1 2", "-", "+",
    };
    static const int bases[] = { 0, 10, 16, 8, 2, 36 };

    for (size_t i = 0; i < sizeof(samples) / sizeof(samples[0]); i++) {
        for (size_t j = 0; j < sizeof(bases) / sizeof(bases[0]); j++) {
            char *mine_end;
            char *host_end;
            long mine = mx_strtol(samples[i], &mine_end, bases[j]);
            long host = strtol(samples[i], &host_end, bases[j]);
            unsigned long umine = mx_strtoul(samples[i], &mine_end, bases[j]);
            unsigned long uhost = strtoul(samples[i], &host_end, bases[j]);

            CHECK(mine == host);
            CHECK(umine == uhost);
            CHECK(mine_end - samples[i] == host_end - samples[i]);
        }
    }

    CHECK(mx_atoi("  123xyz") == 123);
    CHECK(mx_abs(-5) == 5);
    CHECK(mx_abs(5) == 5);
    CHECK(mx_labs(-5000000000L) == 5000000000L);
}

static void test_format(void)
{
    char mine[128];
    char host[128];

#define COMPARE(...) \
    do { \
        int mine_count = mx_snprintf(mine, sizeof(mine), __VA_ARGS__); \
        int host_count = snprintf(host, sizeof(host), __VA_ARGS__); \
        CHECK_STR(mine, host); \
        CHECK(mine_count == host_count); \
    } while (0)

    COMPARE("plain text");
    COMPARE("%d|%5d|%-5d|%05d|%d", 42, 42, 42, 42, -42);
    COMPARE("%5d|%-5d|%05d", -42, -42, -42);
    COMPARE("%u|%x|%X|%08x", 4000000000u, 255u, 255u, 255u);
    COMPARE("%ld|%lu|%lx", -5000000000L, 5000000000UL, 0xdeadbeefcafeUL);
    COMPARE("%lld|%llu|%llx", (long long)-1, (unsigned long long)-1, 0x123456789abcdefULL);
    COMPARE("%zu|%zx", (size_t)99, (size_t)255);
    COMPARE("%s|%10s|%-10s|", "abc", "abc", "abc");
    COMPARE("%c|%3c|%-3c|", 'x', 'y', 'z');
    COMPARE("100%%");
    COMPARE("%d", 0);
    COMPARE("%u", 0u);
    COMPARE("%lld", (long long)(-9223372036854775807LL - 1));
    COMPARE("%s", "");
    COMPARE("%016lx-%016lx", 0x100000UL, 0xfffffUL);

    {
        int count = mx_snprintf(mine, 8, "%s", "0123456789");
        CHECK(count == 10);
        CHECK_STR(mine, "0123456");
        CHECK(mx_snprintf(mine, 0, "%s", "abc") == 3);
        CHECK(mx_snprintf(mine, 1, "%s", "abc") == 3);
        CHECK(mine[0] == '\0');
    }
    CHECK(mx_snprintf(mine, sizeof(mine), "%s", (char *)NULL) == 6);
    CHECK_STR(mine, "(null)");
}

static void test_time(void)
{
    static const long long samples[] = {
        0, 1, 59, 86399, 86400, 951782400LL, 951868800LL, 1000000000LL, 1700000000LL, 1791000000LL, 2147483647LL,
        4102444800LL, 253402300799LL, -1, -86400, -2208988800LL, 1709164800LL,
    };

    for (size_t i = 0; i < sizeof(samples) / sizeof(samples[0]); i++) {
        struct mx_tm mine;
        struct tm host;
        time_t seconds = (time_t)samples[i];
        long long mine_seconds = samples[i];

        mx_gmtime_r(&mine_seconds, &mine);
        gmtime_r(&seconds, &host);
        CHECK(mine.tm_sec == host.tm_sec);
        CHECK(mine.tm_min == host.tm_min);
        CHECK(mine.tm_hour == host.tm_hour);
        CHECK(mine.tm_mday == host.tm_mday);
        CHECK(mine.tm_mon == host.tm_mon);
        CHECK(mine.tm_year == host.tm_year);
        CHECK(mine.tm_wday == host.tm_wday);
        CHECK(mine.tm_yday == host.tm_yday);

        CHECK(mx_timegm(&mine) == samples[i]);
    }

    for (long long step = -200000000LL; step < 4000000000LL; step += 7919123LL) {
        struct mx_tm mine;
        struct tm host;
        time_t seconds = (time_t)step;
        long long mine_seconds = step;

        mx_gmtime_r(&mine_seconds, &mine);
        gmtime_r(&seconds, &host);
        CHECK(mine.tm_year == host.tm_year && mine.tm_mon == host.tm_mon && mine.tm_mday == host.tm_mday);
        CHECK(mine.tm_wday == host.tm_wday && mine.tm_yday == host.tm_yday);
        CHECK(mx_timegm(&mine) == step);
    }

    {
        struct mx_tm mine = { .tm_sec = 75, .tm_min = 59, .tm_hour = 23, .tm_mday = 31, .tm_mon = 12, .tm_year = 99 };
        struct tm host = { .tm_sec = 75, .tm_min = 59, .tm_hour = 23, .tm_mday = 31, .tm_mon = 12, .tm_year = 99 };

        CHECK(mx_timegm(&mine) == (long long)timegm(&host));
        CHECK(mine.tm_year == host.tm_year && mine.tm_mon == host.tm_mon && mine.tm_mday == host.tm_mday);
    }
}

struct allocation {
    uint8_t *pointer;
    size_t size;
    uint8_t pattern;
};

static uint32_t random_state = 12345;

static uint32_t next_random(void)
{
    random_state = random_state * 1664525u + 1013904223u;
    return random_state >> 8;
}

static void verify_allocation(const struct allocation *allocation)
{
    for (size_t i = 0; i < allocation->size; i++) {
        if (allocation->pointer[i] != allocation->pattern) {
            CHECK(allocation->pointer[i] == allocation->pattern);
            return;
        }
    }
}

static void test_malloc(void)
{
    enum { SLOTS = 64, ARENA = 1 << 20 };
    struct allocation slots[SLOTS] = { { 0 } };
    struct mx_malloc_info info;
    void *arena = malloc(ARENA);
    size_t initial_free;

    mx___malloc_init(arena, ARENA);
    mx___malloc_get_info(&info);
    CHECK(info.block_count == 1);
    CHECK(info.used_bytes == 0);
    initial_free = info.free_bytes;

    CHECK(mx_malloc(ARENA * 2) == NULL);
    CHECK(((uintptr_t)mx_malloc(1) & 15) == 0);

    mx___malloc_init(arena, ARENA);

    for (int round = 0; round < 20000; round++) {
        struct allocation *slot = &slots[next_random() % SLOTS];
        uint32_t action = next_random() % 4;

        if (slot->pointer != NULL)
            verify_allocation(slot);

        if (slot->pointer == NULL || action == 0) {
            size_t size = 1 + next_random() % 3000;

            if (slot->pointer != NULL) {
                mx_free(slot->pointer);
                slot->pointer = NULL;
            }
            slot->pointer = mx_malloc(size);
            if (slot->pointer != NULL) {
                slot->size = size;
                slot->pattern = (uint8_t)next_random();
                memset(slot->pointer, slot->pattern, size);
                CHECK(((uintptr_t)slot->pointer & 15) == 0);
            }
        } else if (action == 1) {
            size_t size = 1 + next_random() % 3000;
            size_t keep = size < slot->size ? size : slot->size;
            uint8_t *grown = mx_realloc(slot->pointer, size);

            if (grown != NULL) {
                for (size_t i = 0; i < keep; i++)
                    CHECK(grown[i] == slot->pattern);
                slot->pointer = grown;
                slot->size = size;
                memset(grown, slot->pattern, size);
            }
        } else {
            mx_free(slot->pointer);
            slot->pointer = NULL;
        }
    }

    for (int i = 0; i < SLOTS; i++) {
        for (int j = i + 1; j < SLOTS; j++) {
            if (slots[i].pointer != NULL && slots[j].pointer != NULL) {
                CHECK(slots[i].pointer + slots[i].size <= slots[j].pointer ||
                      slots[j].pointer + slots[j].size <= slots[i].pointer);
            }
        }
    }

    for (int i = 0; i < SLOTS; i++) {
        if (slots[i].pointer != NULL) {
            verify_allocation(&slots[i]);
            mx_free(slots[i].pointer);
        }
    }

    mx___malloc_get_info(&info);
    CHECK(info.block_count == 1);
    CHECK(info.used_bytes == 0);
    CHECK(info.free_bytes == initial_free);
    CHECK(info.largest_free_block == initial_free);

    {
        uint8_t *zeroed = mx_calloc(100, 8);

        CHECK(zeroed != NULL);
        for (int i = 0; i < 800; i++)
            CHECK(zeroed[i] == 0);
        CHECK(mx_calloc(SIZE_MAX / 2, 4) == NULL);
        mx_free(zeroed);
        mx_free(NULL);
        CHECK(mx_realloc(NULL, 10) != NULL);
    }
    free(arena);
}

int main(void)
{
    test_string();
    test_ctype();
    test_convert();
    test_format();
    test_time();
    test_malloc();
    CHECK_REPORT("libc");
}
