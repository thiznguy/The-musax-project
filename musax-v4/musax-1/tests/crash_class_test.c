#include <string.h>
#include <musax/crash.h>
#include "check.h"

#define DRIVER_START 0x104000ULL
#define DRIVER_END 0x106000ULL

int main(void)
{
    CHECK(crash_classify_exception(18, 0x101000, DRIVER_START, DRIVER_END) == CRASH_HARDWARE);
    CHECK(crash_classify_exception(18, DRIVER_START + 4, DRIVER_START, DRIVER_END) == CRASH_HARDWARE);
    CHECK(crash_classify_exception(2, 0x101000, DRIVER_START, DRIVER_END) == CRASH_HARDWARE);

    CHECK(crash_classify_exception(14, DRIVER_START, DRIVER_START, DRIVER_END) == CRASH_DRIVER);
    CHECK(crash_classify_exception(13, DRIVER_END - 1, DRIVER_START, DRIVER_END) == CRASH_DRIVER);
    CHECK(crash_classify_exception(6, DRIVER_START + 0x100, DRIVER_START, DRIVER_END) == CRASH_DRIVER);

    CHECK(crash_classify_exception(14, DRIVER_START - 1, DRIVER_START, DRIVER_END) == CRASH_UNCLASSIFIED);
    CHECK(crash_classify_exception(14, DRIVER_END, DRIVER_START, DRIVER_END) == CRASH_UNCLASSIFIED);
    CHECK(crash_classify_exception(0, 0x200000, DRIVER_START, DRIVER_END) == CRASH_UNCLASSIFIED);
    CHECK(crash_classify_exception(8, 0x101000, DRIVER_START, DRIVER_END) == CRASH_UNCLASSIFIED);

    CHECK(crash_classify_exception(14, 0, 0, 0) == CRASH_UNCLASSIFIED);

    CHECK_STR(crash_class_code(CRASH_DRIVER), "09877KL");
    CHECK_STR(crash_class_code(CRASH_HARDWARE), "56694MU");
    CHECK(crash_class_code(CRASH_UNCLASSIFIED) == NULL);
    CHECK_STR(CRASH_TITLE, "oppsies... your pc seems to have crashed");

    CHECK_REPORT("crash_class");
}
