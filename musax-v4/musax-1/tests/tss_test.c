#include <stddef.h>
#include <stdint.h>
#include <musax/arch/x86_64/tss.h>
#include "check.h"

int main(void)
{
    uint64_t descriptor[2];

    CHECK(sizeof(struct tss) == 104);
    CHECK(offsetof(struct tss, rsp) == 4);
    CHECK(offsetof(struct tss, ist) == 36);
    CHECK(offsetof(struct tss, iomap_base) == 102);

    tss_encode_descriptor(descriptor, 0x0000123456789ABCULL, 103);
    CHECK((descriptor[0] & 0xFFFF) == 103);
    CHECK(((descriptor[0] >> 16) & 0xFFFFFF) == 0x789ABC);
    CHECK(((descriptor[0] >> 40) & 0xFF) == 0x89);
    CHECK(((descriptor[0] >> 48) & 0xF) == 0);
    CHECK(((descriptor[0] >> 52) & 0xF) == 0);
    CHECK(((descriptor[0] >> 56) & 0xFF) == 0x56);
    CHECK(descriptor[1] == 0x1234);

    tss_encode_descriptor(descriptor, 0xFFFFFFFF80000000ULL, 0x1FFFF);
    CHECK((descriptor[0] & 0xFFFF) == 0xFFFF);
    CHECK(((descriptor[0] >> 48) & 0xF) == 1);
    CHECK(((descriptor[0] >> 16) & 0xFFFFFF) == 0x000000);
    CHECK(((descriptor[0] >> 56) & 0xFF) == 0x80);
    CHECK(descriptor[1] == 0xFFFFFFFFULL);

    CHECK_REPORT("tss");
}
