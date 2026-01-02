#pragma once

/// @file userver/proto-structs/date.hpp
/// @brief @copybrief proto_structs::Date

#include <chrono>
#include <cstdint>

#include <userver/utils/datetime/date.hpp>
#include <userver/utils/impl/internal_tag_fwd.hpp>

namespace google::type {
class Date;
}  // namespace google::type

USERVER_NAMESPACE_BEGIN

namespace proto_structs {

/// @brief Type to represent `google.type.Date` in proto structs.
///
/// This type is organized in the same way as its protobuf counterpart and allows for the same range of values
/// described here https://github.com/googleapis/googleapis/blob/master/google/type/date.proto :
/// * a full date, with non-zero year, month, and day values
/// * a year and month value, with a zero day, such as a credit card expiration date
/// * a year on its own, with zero month and day values
/// * a month and day value, with a zero year, such as an anniversary
///
/// The year part of the date must be in the `[1, 9999]` range. The month should be in the `[1, 12]` range.
/// The day should be in the `[1, 31]` range. Altogether, year, month and day must represent a valid date.
class Date {
public:
    using ProtobufMessage = ::google::type::Date;

    /// @brief Creates empty date with all components unset.
    constexpr Date() = default;

    /// @brief Creates date.
    /// @throws ValueError if @a year, @a month and @a day do not represent a meaningful and valid date.
    constexpr Date(
        const std::optional<std::chrono::year>& year,
        const std::optional<std::chrono::month>& month,
        const std::optional<std::chrono::day>& day
    )
        : year_(year),
          month_(month),
          day_(day)
    {
        if (!IsValid(year_, month_, day_)) {
            ThrowError(YearNum(), MonthNum(), DayNum(), "invalid or out of range");
        }
    }

    /// @brief Creates full date.
    /// @throws ValueError if @a ymd is not valid or outside the allowed range.
    constexpr Date(const std::chrono::year_month_day& ymd)
        : Date(ymd.year(), ymd.month(), ymd.day())
    {}

    /// @brief Creates date without a day.
    /// @throws ValueError if @a ym is not valid or outside the allowed range.
    constexpr Date(const std::chrono::year_month& ym)
        : Date(ym.year(), ym.month(), std::nullopt)
    {}

    /// @brief Creates date without a year.
    /// @throws ValueError if @a md is not valid.
    constexpr Date(const std::chrono::month_day& md)
        : Date(std::nullopt, md.month(), md.day())
    {}

    /// @brief Creates date holding only year.
    /// @throws ValueError if @a year is outside the allowed range.
    constexpr Date(const std::chrono::year& year)
        : Date(year, std::nullopt, std::nullopt)
    {}

    /// @brief Creates full date.
    /// @throws ValueError if @a date is outside the allowed range.
    constexpr Date(const utils::datetime::Date& date)
        : Date(std::chrono::year_month_day{date.GetSysDays()})
    {}

    /// @brief Creates full date from `std::chrono::sys_days`.
    /// @throws ValueError if @a sys_days is outside the allowed range.
    constexpr Date(const std::chrono::sys_days& sys_days)
        : Date(std::chrono::year_month_day{sys_days})
    {}

    Date(utils::impl::InternalTag, std::int32_t year, std::int32_t month, std::int32_t day);

    /// @brief Returns year.
    [[nodiscard]] constexpr const std::optional<std::chrono::year>& Year() const noexcept { return year_; }

    /// @brief Returns month.
    [[nodiscard]] constexpr const std::optional<std::chrono::month>& Month() const noexcept { return month_; }

    /// @brief Returns day.
    [[nodiscard]] constexpr const std::optional<std::chrono::day>& Day() const noexcept { return day_; }

    /// @brief Returns year as integer.
    /// @note Zero value means that year is not set.
    [[nodiscard]] constexpr std::int32_t YearNum() const noexcept {
        return year_ ? static_cast<std::int32_t>(*year_) : 0;
    }

    /// @brief Returns month as integer.
    /// @note Zero value means that month is not set.
    [[nodiscard]] constexpr std::int32_t MonthNum() const noexcept {
        return month_ ? static_cast<std::int32_t>(static_cast<unsigned>(*month_)) : 0;
    }

    /// @brief Returns day as integer.
    /// @note Zero value means that day is not set.
    [[nodiscard]] constexpr std::int32_t DayNum() const noexcept {
        return day_ ? static_cast<std::int32_t>(static_cast<unsigned>(*day_)) : 0;
    }

    /// @brief Converts date to `std::chrono::year_month_day`.
    /// @throws ValueError if any date component is not set.
    [[nodiscard]] constexpr std::chrono::year_month_day ToChronoDate() const {
        if (HasYearMonthDay()) {
            return *year_ / *month_ / *day_;
        } else {
            ThrowError(YearNum(), MonthNum(), DayNum(), "all date components should be set");
        }
    }

    /// @brief Converts date to `std::chrono::year_month`.
    /// @throws ValueError if year or month is not set.
    [[nodiscard]] constexpr std::chrono::year_month ToChronoYearMonth() const {
        if (HasYearMonth()) {
            return *year_ / *month_;
        } else {
            ThrowError(YearNum(), MonthNum(), DayNum(), "year and month should be set");
        }
    }

    /// @brief Converts date to `std::chrono::month_day`.
    /// @throws ValueError if month or day is not set.
    [[nodiscard]] constexpr std::chrono::month_day ToChronoMonthDay() const {
        if (HasMonthDay()) {
            return *month_ / *day_;
        } else {
            ThrowError(YearNum(), MonthNum(), DayNum(), "month and day should be set");
        }
    }

    /// @brief Converts date to `std::chrono::year`.
    /// @throws ValueError if year is not set.
    [[nodiscard]] constexpr std::chrono::year ToChronoYear() const {
        if (HasYear()) {
            return *year_;
        } else {
            ThrowError(YearNum(), MonthNum(), DayNum(), "year should be set");
        }
    }

    /// @brief Converts date to `std::chrono::sys_days`.
    /// @throws ValueError if year is not set.
    /// @note If day is not set, `1d` is assumed. If month is also not set, `January` is assumed.
    [[nodiscard]] constexpr std::chrono::sys_days ToChronoSysDays() const {
        if (HasYear()) {
            return static_cast<std::chrono::sys_days>(std::chrono::year_month_day{
                *year_ / month_.value_or(std::chrono::January) / day_.value_or(std::chrono::day{1})
            });
        } else {
            ThrowError(YearNum(), MonthNum(), DayNum(), "year should be set");
        }
    }

    /// @brief Converts date to `userver::utils::datetime::Date`.
    /// @throws ValueError if any date component is not set.
    [[nodiscard]] utils::datetime::Date ToUserverDate() const {
        if (HasYearMonthDay()) {
            return utils::datetime::Date{
                static_cast<int>(YearNum()),
                static_cast<int>(MonthNum()),
                static_cast<int>(DayNum())
            };
        } else {
            ThrowError(YearNum(), MonthNum(), DayNum(), "all date components should be set");
        }
    }

    /// @brief Returns `true` if date is empty (i.e. all components are not set).
    [[nodiscard]] constexpr bool IsEmpty() const noexcept { return !year_ && !month_ && !day_; }

    /// @brief Returns `true` if date has all components.
    [[nodiscard]] constexpr bool HasYearMonthDay() const noexcept { return year_ && month_ && day_; }

    /// @brief Returns `true` if date is has year and month.
    [[nodiscard]] constexpr bool HasYearMonth() const noexcept { return year_ && month_; }

    /// @brief Returns `true` if date is has month and day.
    [[nodiscard]] constexpr bool HasMonthDay() const noexcept { return month_ && day_; }

    /// @brief Returns `true` if date is has year.
    [[nodiscard]] constexpr bool HasYear() const noexcept { return year_.has_value(); }

    /// @brief Explicit conversion to `std::chrono::year_month_day`.
    /// @throws ValueError if any date component is not set.
    [[nodiscard]] constexpr explicit operator std::chrono::year_month_day() const { return ToChronoDate(); }

    /// @brief Explicit conversion to `std::chrono::year_month`.
    /// @throws ValueError if year or month is not set.
    [[nodiscard]] constexpr explicit operator std::chrono::year_month() const { return ToChronoYearMonth(); }

    /// @brief Explicit conversion to `std::chrono::month_day`.
    /// @throws ValueError if month or day is not set.
    [[nodiscard]] constexpr explicit operator std::chrono::month_day() const { return ToChronoMonthDay(); }

    /// @brief Explicit conversion to `std::chrono::year`.
    /// @throws ValueError if year is not set.
    [[nodiscard]] constexpr explicit operator std::chrono::year() const { return ToChronoYear(); }

    /// @brief Explicit conversion to `std::chrono::sys_days`.
    /// @throws ValueError if year is not set.
    /// @note If day is not set, `1d` is assumed. If month is also not set, `January` is assumed.
    [[nodiscard]] constexpr explicit operator std::chrono::sys_days() const { return ToChronoSysDays(); }

    /// @brief Explicit conversion to `userver::utils::datetime::Date`.
    /// @throws ValueError if any date component is not set.
    [[nodiscard]] constexpr explicit operator utils::datetime::Date() const { return ToUserverDate(); }

    /// @brief Default three-way comparison operator.
    constexpr auto operator<=>(const Date&) const = default;

    /// @brief Returns `true` if @a year, @a month and @a day represent a meaningful and valid date.
    [[nodiscard]] static constexpr bool IsValid(
        const std::optional<std::chrono::year>& year,
        const std::optional<std::chrono::month>& month,
        const std::optional<std::chrono::day>& day
    ) noexcept {
        if (year) {
            if (static_cast<int>(*year) <= 0 || static_cast<int>(*year) >= 10'000) {
                return false;
            }

            if (month) {
                if (day) {
                    return std::chrono::year_month_day{*year, *month, *day}.ok();
                } else {
                    return std::chrono::year_month{*year, *month}.ok();
                }
            } else if (day) {
                // year-day is invalid
                return false;
            } else {
                return true;
            }
        } else if (month) {
            if (day) {
                return std::chrono::month_day{*month, *day}.ok();
            } else {
                // only month is invalid
                return false;
            }
        } else if (day) {
            // only day is invalid
            return false;
        } else {
            // empty date with all components unset is allowed
            return true;
        }
    }

    [[nodiscard]] static bool IsValid(
        utils::impl::InternalTag,
        std::int32_t year_num,
        std::int32_t month_num,
        std::int32_t day_num
    ) noexcept;

private:
    [[noreturn]] static void ThrowError(std::int32_t year, std::int32_t month, std::int32_t day, const char* reason);

    std::optional<std::chrono::year> year_{};
    std::optional<std::chrono::month> month_{};
    std::optional<std::chrono::day> day_{};
};

}  // namespace proto_structs

USERVER_NAMESPACE_END
