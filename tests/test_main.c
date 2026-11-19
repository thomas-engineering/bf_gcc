/* Host unit tests for the hardware independent parts of the firmware.
 *
 *   make -C tests        (needs a native gcc, not avr-gcc)
 *
 * The AVR build uses -funsigned-char, the tests are built the same way.
 */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "BCD.h"
#include "calendar.h"

static int checks, failures;

#define CHECK(cond) do { \
    checks++; \
    if (!(cond)) { failures++; printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } \
} while (0)

static void test_bcd(void)
{
    for (int v = 0; v <= 99; v++)
        CHECK((uint8_t)CHAR2BCD2((char)v) == (uint8_t)(((v / 10) << 4) | (v % 10)));

    for (int v = 0; v <= 255; v++)
        CHECK(CHAR2BCD3((char)v) == (uint16_t)(((v / 100) << 8) | (((v / 10) % 10) << 4) | (v % 10)));
}

/* the nested if/else chain that used to be copied into RTC.c */
static bool leap_reference(uint16_t y)
{
    if (!(y & 0x0003))
        if (y % 100 == 0)
            return y % 400 == 0;
        else
            return true;
    else
        return false;
}

static void test_calendar(void)
{
    for (uint16_t y = 0; y < 10000; y++)
        CHECK(is_leap_year(y) == leap_reference(y));

    CHECK(is_leap_year(0));      /* 2000 */
    CHECK(is_leap_year(4));
    CHECK(!is_leap_year(1));
    CHECK(!is_leap_year(100));   /* 2100 */
    CHECK(is_leap_year(400));

    static const uint8_t len[13] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    for (uint8_t m = 1; m <= 12; m++) {
        CHECK(days_in_month(1, m) == len[m]);                 /* not a leap year */
        CHECK(days_in_month(4, m) == (m == 2 ? 29 : len[m])); /* leap year */
    }
    CHECK(days_in_month(0, 2) == 29);
    CHECK(days_in_month(100, 2) == 28);
    CHECK(days_in_month(2000, 0) == 0);
    CHECK(days_in_month(2000, 13) == 0);
}

int main(void)
{
    test_bcd();
    test_calendar();
    printf("%d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
