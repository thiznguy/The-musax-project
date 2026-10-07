#ifndef TESTS_CHECK_H
#define TESTS_CHECK_H

#include <stdio.h>

static int check_failures;
static int check_total;

#define CHECK(condition) \
    do { \
        check_total++; \
        if (!(condition)) { \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); \
            check_failures++; \
        } \
    } while (0)

#define CHECK_STR(actual, expected) \
    do { \
        const char *actual_value = (actual); \
        const char *expected_value = (expected); \
        check_total++; \
        if (strcmp(actual_value, expected_value) != 0) { \
            printf("FAIL %s:%d: got [%s] want [%s]\n", __FILE__, __LINE__, actual_value, expected_value); \
            check_failures++; \
        } \
    } while (0)

#define CHECK_REPORT(name) \
    do { \
        printf("%s: %d checks, %d failures\n", name, check_total, check_failures); \
        return check_failures != 0; \
    } while (0)

#endif
