#include <stdbool.h>
#include <stdint.h>
#include <musax/arch/x86_64/io.h>
#include <musax/drivers/rtc.h>

#define CMOS_INDEX_PORT 0x70
#define CMOS_DATA_PORT 0x71
#define CMOS_NMI_DISABLE 0x80

#define CMOS_SECONDS 0x00
#define CMOS_MINUTES 0x02
#define CMOS_HOURS 0x04
#define CMOS_DAY 0x07
#define CMOS_MONTH 0x08
#define CMOS_YEAR 0x09
#define CMOS_STATUS_A 0x0A
#define CMOS_STATUS_B 0x0B
#define CMOS_CENTURY 0x32

#define CMOS_STATUS_A_UPDATE_IN_PROGRESS 0x80
#define CMOS_STATUS_B_24_HOUR 0x02
#define CMOS_STATUS_B_BINARY 0x04
#define CMOS_HOUR_PM 0x80

struct raw_time {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint8_t year;
    uint8_t century;
};

static uint8_t cmos_read(uint8_t reg)
{
    outb(CMOS_INDEX_PORT, (uint8_t)(CMOS_NMI_DISABLE | reg));
    return inb(CMOS_DATA_PORT);
}

static void read_raw(struct raw_time *raw)
{
    while (cmos_read(CMOS_STATUS_A) & CMOS_STATUS_A_UPDATE_IN_PROGRESS)
        ;
    raw->second = cmos_read(CMOS_SECONDS);
    raw->minute = cmos_read(CMOS_MINUTES);
    raw->hour = cmos_read(CMOS_HOURS);
    raw->day = cmos_read(CMOS_DAY);
    raw->month = cmos_read(CMOS_MONTH);
    raw->year = cmos_read(CMOS_YEAR);
    raw->century = cmos_read(CMOS_CENTURY);
}

static bool raw_equal(const struct raw_time *a, const struct raw_time *b)
{
    return a->second == b->second && a->minute == b->minute && a->hour == b->hour && a->day == b->day &&
           a->month == b->month && a->year == b->year && a->century == b->century;
}

static uint8_t from_bcd(uint8_t value)
{
    return (uint8_t)((value >> 4) * 10 + (value & 0x0F));
}

bool rtc_read(struct rtc_time *time)
{
    struct raw_time first;
    struct raw_time second;
    uint8_t status_b = cmos_read(CMOS_STATUS_B);
    bool binary = (status_b & CMOS_STATUS_B_BINARY) != 0;
    bool pm;
    uint8_t century;
    uint8_t hour;

    read_raw(&first);
    do {
        second = first;
        read_raw(&first);
    } while (!raw_equal(&first, &second));

    pm = (first.hour & CMOS_HOUR_PM) != 0;
    hour = (uint8_t)(first.hour & ~CMOS_HOUR_PM);
    if (!binary) {
        hour = from_bcd(hour);
        first.second = from_bcd(first.second);
        first.minute = from_bcd(first.minute);
        first.day = from_bcd(first.day);
        first.month = from_bcd(first.month);
        first.year = from_bcd(first.year);
        first.century = from_bcd(first.century);
    }
    if (!(status_b & CMOS_STATUS_B_24_HOUR) && pm)
        hour = (uint8_t)((hour % 12) + 12);

    century = first.century >= 19 && first.century <= 21 ? first.century : 20;

    time->year = (uint16_t)(century * 100 + first.year);
    time->month = first.month;
    time->day = first.day;
    time->hour = hour;
    time->minute = first.minute;
    time->second = first.second;

    return time->month >= 1 && time->month <= 12 && time->day >= 1 && time->day <= 31 && time->hour < 24 &&
           time->minute < 60 && time->second < 61;
}
