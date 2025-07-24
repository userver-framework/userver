#include <userver/utils/datetime/cpp_20_calendar.hpp>

#include <gtest/gtest.h>

#include <userver/utest/assert_macros.hpp>

USERVER_NAMESPACE_BEGIN

TEST(DaysBetweenYears, LeapYear) { EXPECT_EQ(utils::datetime::DaysBetweenYears(2019, 2021).count(), 365 + 366); }
TEST(DaysBetweenYears, YearOne) { EXPECT_EQ(utils::datetime::DaysBetweenYears(1, 1970).count(), 719162); }
TEST(DaysBetweenYears, MaxYear) { EXPECT_EQ(utils::datetime::DaysBetweenYears(1970, 10000).count(), 2932897); }
TEST(DaysBetweenYears, NegativeYearOne) { EXPECT_EQ(utils::datetime::DaysBetweenYears(1970, 1).count(), -719162); }

TEST(DaysInMonth, January) { EXPECT_EQ(utils::datetime::DaysInMonth(1, 2025), date::day(31)); }
TEST(DaysInMonth, February) { EXPECT_EQ(utils::datetime::DaysInMonth(2, 2025), date::day(28)); }
TEST(DaysInMonth, March) { EXPECT_EQ(utils::datetime::DaysInMonth(3, 2025), date::day(31)); }
TEST(DaysInMonth, April) { EXPECT_EQ(utils::datetime::DaysInMonth(4, 2025), date::day(30)); }
TEST(DaysInMonth, May) { EXPECT_EQ(utils::datetime::DaysInMonth(5, 2025), date::day(31)); }
TEST(DaysInMonth, June) { EXPECT_EQ(utils::datetime::DaysInMonth(6, 2025), date::day(30)); }
TEST(DaysInMonth, July) { EXPECT_EQ(utils::datetime::DaysInMonth(7, 2025), date::day(31)); }
TEST(DaysInMonth, August) { EXPECT_EQ(utils::datetime::DaysInMonth(8, 2025), date::day(31)); }
TEST(DaysInMonth, September) { EXPECT_EQ(utils::datetime::DaysInMonth(9, 2025), date::day(30)); }
TEST(DaysInMonth, October) { EXPECT_EQ(utils::datetime::DaysInMonth(10, 2025), date::day(31)); }
TEST(DaysInMonth, November) { EXPECT_EQ(utils::datetime::DaysInMonth(11, 2025), date::day(30)); }
TEST(DaysInMonth, December) { EXPECT_EQ(utils::datetime::DaysInMonth(12, 2025), date::day(31)); }

TEST(DaysInMonth, ZeroMonth) {
    EXPECT_UINVARIANT_FAILURE_MSG(utils::datetime::DaysInMonth(0, 2025), "Month must be between 1 and 12");
}
TEST(DaysInMonth, BigMonth) {
    EXPECT_UINVARIANT_FAILURE_MSG(utils::datetime::DaysInMonth(13, 2025), "Month must be between 1 and 12");
}

TEST(DaysInMonth, LeapJanuary) { EXPECT_EQ(utils::datetime::DaysInMonth(1, 2024), date::day(31)); }
TEST(DaysInMonth, LeapFebruary) { EXPECT_EQ(utils::datetime::DaysInMonth(2, 2024), date::day(29)); }
TEST(DaysInMonth, LeapMarch) { EXPECT_EQ(utils::datetime::DaysInMonth(3, 2024), date::day(31)); }
TEST(DaysInMonth, LeapApril) { EXPECT_EQ(utils::datetime::DaysInMonth(4, 2024), date::day(30)); }
TEST(DaysInMonth, LeapMay) { EXPECT_EQ(utils::datetime::DaysInMonth(5, 2024), date::day(31)); }
TEST(DaysInMonth, LeapJune) { EXPECT_EQ(utils::datetime::DaysInMonth(6, 2024), date::day(30)); }
TEST(DaysInMonth, LeapJuly) { EXPECT_EQ(utils::datetime::DaysInMonth(7, 2024), date::day(31)); }
TEST(DaysInMonth, LeapAugust) { EXPECT_EQ(utils::datetime::DaysInMonth(8, 2024), date::day(31)); }
TEST(DaysInMonth, LeapSeptember) { EXPECT_EQ(utils::datetime::DaysInMonth(9, 2024), date::day(30)); }
TEST(DaysInMonth, LeapOctober) { EXPECT_EQ(utils::datetime::DaysInMonth(10, 2024), date::day(31)); }
TEST(DaysInMonth, LeapNovember) { EXPECT_EQ(utils::datetime::DaysInMonth(11, 2024), date::day(30)); }
TEST(DaysInMonth, LeapDecember) { EXPECT_EQ(utils::datetime::DaysInMonth(12, 2024), date::day(31)); }

USERVER_NAMESPACE_END
