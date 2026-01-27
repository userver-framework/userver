#include <gtest/gtest.h>

#include <userver/proto-structs/date.hpp>
#include <userver/proto-structs/exceptions.hpp>
#include <userver/utest/assert_macros.hpp>
#include <userver/utils/impl/internal_tag.hpp>

using namespace std::literals::chrono_literals;

USERVER_NAMESPACE_BEGIN

namespace proto_structs::tests {

TEST(DateTest, IsValid) {
    EXPECT_FALSE(Date::IsValid(2025y, std::nullopt, 10d));
    EXPECT_FALSE(Date::IsValid(std::nullopt, std::nullopt, 10d));
    EXPECT_FALSE(Date::IsValid(std::nullopt, std::chrono::March, std::nullopt));
    EXPECT_FALSE(Date::IsValid(0y, std::chrono::January, 1d));
    EXPECT_FALSE(Date::IsValid(2025y, std::chrono::month{0}, 1d));
    EXPECT_FALSE(Date::IsValid(2025y, std::chrono::January, std::chrono::day{0}));
    EXPECT_FALSE(Date::IsValid(2025y, std::chrono::June, 31d));
    EXPECT_FALSE(Date::IsValid(std::nullopt, std::chrono::June, 31d));
    EXPECT_FALSE(Date::IsValid(2025y, std::chrono::February, 29d));
    EXPECT_FALSE(Date::IsValid(10'000y, std::chrono::January, 1d));
    EXPECT_FALSE(Date::IsValid(10'000y, std::nullopt, std::nullopt));
    EXPECT_FALSE(Date::IsValid(-1y, std::chrono::January, 1d));
    EXPECT_FALSE(Date::IsValid(utils::impl::InternalTag{}, 2025, 0, 10));
    EXPECT_FALSE(Date::IsValid(utils::impl::InternalTag{}, 0, 0, 10));
    EXPECT_FALSE(Date::IsValid(utils::impl::InternalTag{}, 0, 3, 0));
    EXPECT_FALSE(Date::IsValid(utils::impl::InternalTag{}, -1, 1, 1));
    EXPECT_FALSE(Date::IsValid(utils::impl::InternalTag{}, 10'000, 1, 1));
    EXPECT_FALSE(Date::IsValid(utils::impl::InternalTag{}, 2025, -10, 1));
    EXPECT_FALSE(Date::IsValid(utils::impl::InternalTag{}, 2025, 10000, 1));
    EXPECT_FALSE(Date::IsValid(utils::impl::InternalTag{}, 2025, 1, -10));
    EXPECT_FALSE(Date::IsValid(utils::impl::InternalTag{}, 2025, 1, 10000));

    EXPECT_THROW(Date(2025y, std::nullopt, 10d), ValueError);
    EXPECT_THROW(Date(2025y / 2 / 29), ValueError);
    EXPECT_THROW(Date(std::chrono::year_month{2025y, std::chrono::month{20}}), ValueError);
    EXPECT_THROW(Date(10000y), ValueError);
    EXPECT_THROW(Date(std::chrono::month_day{std::chrono::June, 31d}), ValueError);
    EXPECT_THROW(Date(utils::datetime::Date{10000, 1, 1}), ValueError);
    EXPECT_THROW(Date(utils::impl::InternalTag{}, 2025, 0, 10), ValueError);
    EXPECT_THROW(
        Date(std::chrono::time_point_cast<
             std::chrono::days>(std::chrono::system_clock::time_point{std::chrono::years(9000 /* +epoch */)})),
        ValueError
    );
    UEXPECT_THROW_MSG(Date(2025y, std::chrono::February, 29d), ValueError, "2025y/2m/29d");
    UEXPECT_THROW_MSG(Date(utils::impl::InternalTag{}, 10, -10, 5), ValueError, "10y/-10m/5d");

    EXPECT_TRUE(Date::IsValid(std::nullopt, std::nullopt, std::nullopt));
    EXPECT_TRUE(Date::IsValid(2024y, std::chrono::February, 29d));
    EXPECT_TRUE(Date::IsValid(2025y, std::chrono::March, std::nullopt));
    EXPECT_TRUE(Date::IsValid(2025y, std::nullopt, std::nullopt));
    EXPECT_TRUE(Date::IsValid(std::nullopt, std::chrono::March, 30d));
    EXPECT_TRUE(Date::IsValid(std::nullopt, std::chrono::February, 29d));
}

TEST(DateTest, Conversions) {
    Date d{std::nullopt, std::nullopt, std::nullopt};

    EXPECT_TRUE(d.IsEmpty());
    EXPECT_FALSE(d.HasYearMonthDay());
    EXPECT_FALSE(d.HasYearMonth());
    EXPECT_FALSE(d.HasMonthDay());
    EXPECT_FALSE(d.HasYear());
    EXPECT_FALSE(d.Year());
    EXPECT_FALSE(d.Month());
    EXPECT_FALSE(d.Day());
    EXPECT_EQ(d.YearNum(), 0);
    EXPECT_EQ(d.MonthNum(), 0);
    EXPECT_EQ(d.DayNum(), 0);
    EXPECT_THROW([[maybe_unused]] auto val = d.ToChronoDate(), ValueError);
    EXPECT_THROW([[maybe_unused]] auto val = static_cast<std::chrono::year_month_day>(d), ValueError);
    EXPECT_THROW([[maybe_unused]] auto val = d.ToUserverDate(), ValueError);
    EXPECT_THROW([[maybe_unused]] auto val = static_cast<utils::datetime::Date>(d), ValueError);
    EXPECT_THROW([[maybe_unused]] auto val = d.ToChronoYearMonth(), ValueError);
    EXPECT_THROW([[maybe_unused]] auto val = static_cast<std::chrono::year_month>(d), ValueError);
    EXPECT_THROW([[maybe_unused]] auto val = d.ToChronoMonthDay(), ValueError);
    EXPECT_THROW([[maybe_unused]] auto val = static_cast<std::chrono::month_day>(d), ValueError);
    EXPECT_THROW([[maybe_unused]] auto val = d.ToChronoYear(), ValueError);
    EXPECT_THROW([[maybe_unused]] auto val = static_cast<std::chrono::year>(d), ValueError);
    EXPECT_THROW([[maybe_unused]] auto val = d.ToChronoSysDays(), ValueError);
    EXPECT_THROW([[maybe_unused]] auto val = static_cast<std::chrono::sys_days>(d), ValueError);

    d = Date{};

    EXPECT_TRUE(d.IsEmpty());
    EXPECT_FALSE(d.HasYearMonthDay());
    EXPECT_FALSE(d.HasYearMonth());
    EXPECT_FALSE(d.HasMonthDay());
    EXPECT_FALSE(d.HasYear());
    EXPECT_FALSE(d.Year());
    EXPECT_FALSE(d.Month());
    EXPECT_FALSE(d.Day());
    EXPECT_EQ(d.YearNum(), 0);
    EXPECT_EQ(d.MonthNum(), 0);
    EXPECT_EQ(d.DayNum(), 0);

    d = 2019y / std::chrono::May / 28d;

    EXPECT_FALSE(d.IsEmpty());
    ASSERT_TRUE(d.HasYearMonthDay());
    ASSERT_TRUE(d.HasYearMonth());
    ASSERT_TRUE(d.HasMonthDay());
    ASSERT_TRUE(d.HasYear());
    EXPECT_EQ(*d.Year(), 2019y);
    EXPECT_EQ(*d.Month(), std::chrono::May);
    EXPECT_EQ(*d.Day(), 28d);
    EXPECT_EQ(d.YearNum(), 2019);
    EXPECT_EQ(d.MonthNum(), 5);
    EXPECT_EQ(d.DayNum(), 28);
    EXPECT_EQ(d.ToChronoDate(), 2019y / std::chrono::May / 28d);
    EXPECT_EQ(static_cast<std::chrono::year_month_day>(d), 2019y / std::chrono::May / 28d);
    EXPECT_EQ(d.ToUserverDate(), utils::datetime::Date(2019, 5, 28));
    EXPECT_EQ(static_cast<utils::datetime::Date>(d), utils::datetime::Date(2019, 5, 28));
    EXPECT_EQ(d.ToChronoYearMonth(), 2019y / std::chrono::May);
    EXPECT_EQ(static_cast<std::chrono::year_month>(d), 2019y / std::chrono::May);
    EXPECT_EQ(d.ToChronoMonthDay(), std::chrono::May / 28d);
    EXPECT_EQ(static_cast<std::chrono::month_day>(d), std::chrono::May / 28d);
    EXPECT_EQ(d.ToChronoYear(), 2019y);
    EXPECT_EQ(static_cast<std::chrono::year>(d), 2019y);
    EXPECT_EQ(d.ToChronoSysDays(), static_cast<std::chrono::sys_days>(2019y / std::chrono::May / 28d));
    EXPECT_EQ(
        static_cast<std::chrono::sys_days>(d),
        static_cast<std::chrono::sys_days>(2019y / std::chrono::May / 28d)
    );

    d = 2019y / std::chrono::May;

    EXPECT_FALSE(d.IsEmpty());
    EXPECT_FALSE(d.HasYearMonthDay());
    ASSERT_TRUE(d.HasYearMonth());
    EXPECT_FALSE(d.HasMonthDay());
    ASSERT_TRUE(d.HasYear());
    EXPECT_THROW([[maybe_unused]] auto val = d.ToChronoDate(), ValueError);
    EXPECT_THROW([[maybe_unused]] auto val = d.ToUserverDate(), ValueError);
    EXPECT_EQ(d.ToChronoYearMonth(), 2019y / std::chrono::May);
    EXPECT_THROW([[maybe_unused]] auto val = d.ToChronoMonthDay(), ValueError);
    EXPECT_EQ(d.ToChronoYear(), 2019y);
    EXPECT_EQ(d.ToChronoSysDays(), static_cast<std::chrono::sys_days>(2019y / std::chrono::May / 1d));
    EXPECT_EQ(static_cast<std::chrono::sys_days>(d), static_cast<std::chrono::sys_days>(2019y / std::chrono::May / 1d));

    d = std::chrono::May / 28d;

    EXPECT_FALSE(d.IsEmpty());
    EXPECT_FALSE(d.HasYearMonthDay());
    EXPECT_FALSE(d.HasYearMonth());
    ASSERT_TRUE(d.HasMonthDay());
    EXPECT_FALSE(d.HasYear());
    EXPECT_THROW([[maybe_unused]] auto val = d.ToChronoDate(), ValueError);
    EXPECT_THROW([[maybe_unused]] auto val = d.ToUserverDate(), ValueError);
    EXPECT_THROW([[maybe_unused]] auto val = d.ToChronoYearMonth(), ValueError);
    EXPECT_EQ(d.ToChronoMonthDay(), std::chrono::May / 28d);
    EXPECT_THROW([[maybe_unused]] auto val = d.ToChronoYear(), ValueError);
    EXPECT_THROW([[maybe_unused]] auto val = d.ToChronoSysDays(), ValueError);

    d = 2019y;

    EXPECT_FALSE(d.IsEmpty());
    EXPECT_FALSE(d.HasYearMonthDay());
    EXPECT_FALSE(d.HasYearMonth());
    EXPECT_FALSE(d.HasMonthDay());
    ASSERT_TRUE(d.HasYear());
    EXPECT_THROW([[maybe_unused]] auto val = d.ToChronoDate(), ValueError);
    EXPECT_THROW([[maybe_unused]] auto val = d.ToUserverDate(), ValueError);
    EXPECT_THROW([[maybe_unused]] auto val = d.ToChronoYearMonth(), ValueError);
    EXPECT_THROW([[maybe_unused]] auto val = d.ToChronoMonthDay(), ValueError);
    EXPECT_EQ(d.ToChronoYear(), 2019y);
    EXPECT_EQ(d.ToChronoSysDays(), static_cast<std::chrono::sys_days>(2019y / std::chrono::January / 1d));
    EXPECT_EQ(
        static_cast<std::chrono::sys_days>(d),
        static_cast<std::chrono::sys_days>(2019y / std::chrono::January / 1d)
    );

    d = utils::datetime::Date(2019, 5, 28);

    EXPECT_FALSE(d.IsEmpty());
    ASSERT_TRUE(d.HasYearMonthDay());
    ASSERT_TRUE(d.HasYearMonth());
    ASSERT_TRUE(d.HasMonthDay());
    ASSERT_TRUE(d.HasYear());
    EXPECT_EQ(d.ToChronoDate(), 2019y / std::chrono::May / 28d);
    EXPECT_EQ(d.ToUserverDate(), utils::datetime::Date(2019, 5, 28));
    EXPECT_EQ(d.ToChronoYearMonth(), 2019y / std::chrono::May);
    EXPECT_EQ(d.ToChronoMonthDay(), std::chrono::May / 28d);
    EXPECT_EQ(d.ToChronoYear(), 2019y);
    EXPECT_EQ(d.ToChronoSysDays(), static_cast<std::chrono::sys_days>(2019y / std::chrono::May / 28d));
    EXPECT_EQ(
        static_cast<std::chrono::sys_days>(d),
        static_cast<std::chrono::sys_days>(2019y / std::chrono::May / 28d)
    );

    d = static_cast<std::chrono::sys_days>(2019y / std::chrono::May / 28d);

    EXPECT_FALSE(d.IsEmpty());
    ASSERT_TRUE(d.HasYearMonthDay());
    ASSERT_TRUE(d.HasYearMonth());
    ASSERT_TRUE(d.HasMonthDay());
    ASSERT_TRUE(d.HasYear());
    EXPECT_EQ(d.ToChronoDate(), 2019y / std::chrono::May / 28d);
    EXPECT_EQ(d.ToUserverDate(), utils::datetime::Date(2019, 5, 28));
    EXPECT_EQ(d.ToChronoYearMonth(), 2019y / std::chrono::May);
    EXPECT_EQ(d.ToChronoMonthDay(), std::chrono::May / 28d);
    EXPECT_EQ(d.ToChronoYear(), 2019y);
    EXPECT_EQ(d.ToChronoSysDays(), static_cast<std::chrono::sys_days>(2019y / std::chrono::May / 28d));
    EXPECT_EQ(
        static_cast<std::chrono::sys_days>(d),
        static_cast<std::chrono::sys_days>(2019y / std::chrono::May / 28d)
    );

    d = Date{utils::impl::InternalTag{}, 2019, 5, 28};

    EXPECT_FALSE(d.IsEmpty());
    ASSERT_TRUE(d.HasYearMonthDay());
    ASSERT_TRUE(d.HasYearMonth());
    ASSERT_TRUE(d.HasMonthDay());
    ASSERT_TRUE(d.HasYear());
    EXPECT_EQ(*d.Year(), 2019y);
    EXPECT_EQ(*d.Month(), std::chrono::May);
    EXPECT_EQ(*d.Day(), 28d);
    EXPECT_EQ(d.YearNum(), 2019);
    EXPECT_EQ(d.MonthNum(), 5);
    EXPECT_EQ(d.DayNum(), 28);
    EXPECT_EQ(d.ToChronoDate(), 2019y / std::chrono::May / 28d);
    EXPECT_EQ(static_cast<std::chrono::year_month_day>(d), 2019y / std::chrono::May / 28d);
    EXPECT_EQ(d.ToUserverDate(), utils::datetime::Date(2019, 5, 28));
    EXPECT_EQ(static_cast<utils::datetime::Date>(d), utils::datetime::Date(2019, 5, 28));
    EXPECT_EQ(d.ToChronoYearMonth(), 2019y / std::chrono::May);
    EXPECT_EQ(static_cast<std::chrono::year_month>(d), 2019y / std::chrono::May);
    EXPECT_EQ(d.ToChronoMonthDay(), std::chrono::May / 28d);
    EXPECT_EQ(static_cast<std::chrono::month_day>(d), std::chrono::May / 28d);
    EXPECT_EQ(d.ToChronoYear(), 2019y);
    EXPECT_EQ(static_cast<std::chrono::year>(d), 2019y);
    EXPECT_EQ(d.ToChronoSysDays(), static_cast<std::chrono::sys_days>(2019y / std::chrono::May / 28d));
    EXPECT_EQ(
        static_cast<std::chrono::sys_days>(d),
        static_cast<std::chrono::sys_days>(2019y / std::chrono::May / 28d)
    );
}

}  // namespace proto_structs::tests

USERVER_NAMESPACE_END
