#ifndef MUSAX_DRIVERS_RTC_H
#define MUSAX_DRIVERS_RTC_H

#include <stdbool.h>
#include <stdint.h>

struct rtc_time {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
};

/* Reads the CMOS real-time clock; returns false when it reports an invalid date. */
bool rtc_read(struct rtc_time *time);

#endif
