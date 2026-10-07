#include <stdint.h>
#include <time.h>

#define SECONDS_PER_DAY 86400LL
#define UNIX_EPOCH_WEEKDAY 4
#define DAYS_FROM_CIVIL_EPOCH_SHIFT 719468LL

static int64_t days_from_civil(int64_t year, unsigned month, unsigned day)
{
    int64_t era;
    unsigned year_of_era;
    unsigned day_of_year;
    unsigned day_of_era;

    if (month <= 2)
        year--;
    era = (year >= 0 ? year : year - 399) / 400;
    year_of_era = (unsigned)(year - era * 400);
    day_of_year = (153 * (month > 2 ? month - 3 : month + 9) + 2) / 5 + day - 1;
    day_of_era = year_of_era * 365 + year_of_era / 4 - year_of_era / 100 + day_of_year;
    return era * 146097 + (int64_t)day_of_era - DAYS_FROM_CIVIL_EPOCH_SHIFT;
}

struct tm *gmtime_r(const time_t *timer, struct tm *result)
{
    int64_t seconds = *timer;
    int64_t days = seconds / SECONDS_PER_DAY;
    int64_t remainder = seconds % SECONDS_PER_DAY;
    int64_t shifted;
    int64_t era;
    unsigned day_of_era;
    unsigned year_of_era;
    unsigned day_of_year;
    unsigned month_phase;
    unsigned day;
    unsigned month;
    int64_t year;

    if (remainder < 0) {
        remainder += SECONDS_PER_DAY;
        days--;
    }

    shifted = days + DAYS_FROM_CIVIL_EPOCH_SHIFT;
    era = (shifted >= 0 ? shifted : shifted - 146096) / 146097;
    day_of_era = (unsigned)(shifted - era * 146097);
    year_of_era = (day_of_era - day_of_era / 1460 + day_of_era / 36524 - day_of_era / 146096) / 365;
    year = (int64_t)year_of_era + era * 400;
    day_of_year = day_of_era - (365 * year_of_era + year_of_era / 4 - year_of_era / 100);
    month_phase = (5 * day_of_year + 2) / 153;
    day = day_of_year - (153 * month_phase + 2) / 5 + 1;
    month = month_phase < 10 ? month_phase + 3 : month_phase - 9;
    if (month <= 2)
        year++;

    result->tm_sec = (int)(remainder % 60);
    result->tm_min = (int)((remainder / 60) % 60);
    result->tm_hour = (int)(remainder / 3600);
    result->tm_mday = (int)day;
    result->tm_mon = (int)month - 1;
    result->tm_year = (int)(year - 1900);
    result->tm_wday = (int)(((days % 7) + 7 + UNIX_EPOCH_WEEKDAY) % 7);
    result->tm_yday = (int)(days - days_from_civil(year, 1, 1));
    result->tm_isdst = 0;
    return result;
}

time_t timegm(struct tm *broken_down)
{
    int64_t year = (int64_t)broken_down->tm_year + 1900;
    int64_t month = broken_down->tm_mon;
    int64_t days;
    time_t seconds;

    year += month / 12;
    month %= 12;
    if (month < 0) {
        month += 12;
        year--;
    }

    days = days_from_civil(year, (unsigned)month + 1, 1) + (broken_down->tm_mday - 1);
    seconds = days * SECONDS_PER_DAY + (int64_t)broken_down->tm_hour * 3600 + (int64_t)broken_down->tm_min * 60 +
              broken_down->tm_sec;
    gmtime_r(&seconds, broken_down);
    return seconds;
}
