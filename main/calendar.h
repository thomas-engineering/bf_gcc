// calendar.h - date arithmetic without any hardware access
//
// Kept in a header of its own so that it can be unit-tested on the host
// (see tests/). The year is the two-digit year used by the clock (0 = 2000).

#ifndef CALENDAR_H_
#define CALENDAR_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __AVR__
#define CALENDAR_FLASH  __flash     // keep the table out of RAM
#else
#define CALENDAR_FLASH              // host build (unit tests)
#endif

static inline bool is_leap_year(uint16_t year)
{
    if (year % 4) {
        return false;
    }
    if (year % 100) {
        return true;
    }
    return (year % 400) == 0;
}

// Number of days of month 1..12 (0 for an invalid month).
static inline uint8_t days_in_month(uint16_t year, uint8_t month)
{
    static const CALENDAR_FLASH uint8_t length[13] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    if (month < 1 || month > 12) {
        return 0;
    }
    if (month == 2 && is_leap_year(year)) {
        return 29;
    }
    return length[month];
}

#endif /* CALENDAR_H_ */
