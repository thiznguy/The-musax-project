#include <stdbool.h>
#include <time.h>
#include <musax/clock.h>
#include <musax/drivers/rtc.h>

static bool available;
static time_t boot_seconds;

bool clock_now(time_t *seconds_since_epoch)
{
    struct rtc_time rtc;
    struct tm broken_down;

    if (!rtc_read(&rtc))
        return false;

    broken_down.tm_sec = rtc.second;
    broken_down.tm_min = rtc.minute;
    broken_down.tm_hour = rtc.hour;
    broken_down.tm_mday = rtc.day;
    broken_down.tm_mon = rtc.month - 1;
    broken_down.tm_year = rtc.year - 1900;
    broken_down.tm_wday = 0;
    broken_down.tm_yday = 0;
    broken_down.tm_isdst = 0;
    *seconds_since_epoch = timegm(&broken_down);
    return true;
}

bool clock_init(void)
{
    available = clock_now(&boot_seconds);
    return available;
}

bool clock_uptime(time_t *seconds)
{
    time_t now;

    if (!available || !clock_now(&now))
        return false;
    *seconds = now >= boot_seconds ? now - boot_seconds : 0;
    return true;
}
