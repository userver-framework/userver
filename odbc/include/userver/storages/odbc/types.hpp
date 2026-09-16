#pragma once

/// @file userver/storages/odbc/types.hpp
/// @brief Portable value types for standard ODBC SQL types.

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {

/// Owning byte sequence for SQL BINARY, VARBINARY and LONGVARBINARY.
class Bytes final {
public:
    using ValueType = std::uint8_t;
    using Container = std::vector<ValueType>;

    Bytes() = default;
    explicit Bytes(Container bytes);
    Bytes(std::initializer_list<ValueType> bytes);

    const Container& GetBytes() const noexcept;
    std::size_t Size() const noexcept;
    bool IsEmpty() const noexcept;

    bool operator==(const Bytes&) const noexcept = default;

private:
    Container bytes_;
};

/// Timezone-independent Gregorian calendar date in the portable 1..9999 range.
class Date final {
public:
    Date() noexcept;
    Date(std::uint32_t year, std::uint32_t month, std::uint32_t day);

    std::uint32_t GetYear() const noexcept;
    std::uint32_t GetMonth() const noexcept;
    std::uint32_t GetDay() const noexcept;
    std::string ToString() const;

    bool operator==(const Date&) const noexcept = default;

private:
    std::uint16_t year_{1970};
    std::uint8_t month_{1};
    std::uint8_t day_{1};
};

/// Timezone-independent time of day with the portable `SQL_TIME_STRUCT`
/// resolution of one second.
class Time final {
public:
    Time() noexcept = default;
    Time(std::uint32_t hour, std::uint32_t minute, std::uint32_t second);

    std::uint32_t GetHour() const noexcept;
    std::uint32_t GetMinute() const noexcept;
    std::uint32_t GetSecond() const noexcept;
    std::string ToString() const;

    bool operator==(const Time&) const noexcept = default;

private:
    std::uint8_t hour_{0};
    std::uint8_t minute_{0};
    std::uint8_t second_{0};
};

/// Timezone-independent timestamp with nanosecond fraction storage.
///
/// No implicit conversion to or from `std::chrono::system_clock::time_point`
/// is provided because an ODBC TIMESTAMP has no timezone.
class Timestamp final {
public:
    Timestamp() noexcept = default;
    Timestamp(Date date, Time time, std::uint32_t fraction_nanoseconds = 0);
    Timestamp(
        std::uint32_t year,
        std::uint32_t month,
        std::uint32_t day,
        std::uint32_t hour,
        std::uint32_t minute,
        std::uint32_t second,
        std::uint32_t fraction_nanoseconds = 0
    );

    const Date& GetDate() const noexcept;
    const Time& GetTime() const noexcept;
    std::uint32_t GetFractionNanoseconds() const noexcept;
    std::string ToString() const;

    bool operator==(const Timestamp&) const noexcept = default;

private:
    Date date_;
    Time time_;
    std::uint32_t fraction_nanoseconds_{0};
};

/// Exact fixed-point SQL DECIMAL/NUMERIC value.
///
/// Accepted syntax is `[-+]digits` for Scale=0 and
/// `[-+]digits.Scale-digits` otherwise. Exponents, whitespace, NaN and
/// infinities are rejected. Values are canonicalized by removing a leading
/// plus and redundant integer zeroes; negative zero is normalized to positive
/// zero. Exactly Scale fractional digits, including trailing zeroes, are
/// retained. ODBC SQL_NUMERIC_STRUCT limits portable precision to 38 digits.
template <std::size_t Precision, std::size_t Scale>
class Decimal final {
    static_assert(Precision >= 1 && Precision <= 38, "ODBC Decimal precision must be in the range 1..38");
    static_assert(Scale <= Precision, "ODBC Decimal scale must not exceed precision");

public:
    static constexpr std::size_t kPrecision = Precision;
    static constexpr std::size_t kScale = Scale;

    Decimal()
        : representation_{MakeZero()}
    {}

    explicit Decimal(std::string_view representation)
        : representation_{Validate(representation)}
    {}

    std::string_view GetRepresentation() const noexcept;
    static constexpr std::size_t GetPrecision() noexcept { return Precision; }
    static constexpr std::size_t GetScale() noexcept { return Scale; }

    bool operator==(const Decimal&) const noexcept = default;

private:
    static std::string MakeZero();
    static std::string Validate(std::string_view representation);

    std::string representation_;
};

/// @cond
namespace impl {

template <typename T>
struct IsDecimal : std::false_type {};

template <std::size_t Precision, std::size_t Scale>
struct IsDecimal<Decimal<Precision, Scale>> : std::true_type {};

template <typename T>
inline constexpr bool kIsDecimal = IsDecimal<std::remove_cv_t<T>>::value;

}  // namespace impl
/// @endcond

template <std::size_t Precision, std::size_t Scale>
std::string_view Decimal<Precision, Scale>::GetRepresentation() const noexcept {
    return representation_;
}

template <std::size_t Precision, std::size_t Scale>
std::string Decimal<Precision, Scale>::MakeZero() {
    if constexpr (Scale == 0) {
        return "0";
    } else {
        return std::string{"0."} + std::string(Scale, '0');
    }
}

template <std::size_t Precision, std::size_t Scale>
std::string Decimal<Precision, Scale>::Validate(std::string_view representation) {
    if (representation.empty()) {
        throw std::invalid_argument("ODBC Decimal representation must not be empty");
    }

    std::size_t index = representation.front() == '-' || representation.front() == '+' ? 1 : 0;
    const auto integer_begin = index;
    while (index < representation.size() && representation[index] >= '0' && representation[index] <= '9') {
        ++index;
    }
    if (index == integer_begin) {
        throw std::invalid_argument("ODBC Decimal requires at least one integer digit");
    }
    const auto integer_end = index;

    if constexpr (Scale == 0) {
        if (index != representation.size()) {
            throw std::invalid_argument("ODBC Decimal with scale 0 must not contain a fractional part");
        }
    } else {
        if (index == representation.size() || representation[index] != '.') {
            throw std::invalid_argument("ODBC Decimal representation does not contain its declared scale");
        }
        ++index;
        const auto fractional_begin = index;
        while (index < representation.size() && representation[index] >= '0' && representation[index] <= '9') {
            ++index;
        }
        if (index != representation.size() || index - fractional_begin != Scale) {
            throw std::invalid_argument("ODBC Decimal fractional digits do not match its declared scale");
        }
    }

    auto first_significant = integer_begin;
    while (first_significant < integer_end && representation[first_significant] == '0') {
        ++first_significant;
    }
    const auto significant_integer_digits = first_significant == integer_end ? 0 : integer_end - first_significant;
    if (significant_integer_digits > Precision - Scale) {
        throw std::out_of_range("ODBC Decimal magnitude exceeds its declared precision and scale");
    }

    const bool fractional_is_zero = [&] {
        if constexpr (Scale == 0) {
            return true;
        } else {
            for (std::size_t position = integer_end + 1; position < representation.size(); ++position) {
                if (representation[position] != '0') {
                    return false;
                }
            }
            return true;
        }
    }();
    const bool is_zero = significant_integer_digits == 0 && fractional_is_zero;

    std::string result;
    if (!is_zero && representation.front() == '-') {
        result.push_back('-');
    }
    if (first_significant == integer_end) {
        result.push_back('0');
    } else {
        result.append(representation.substr(first_significant, significant_integer_digits));
    }
    if constexpr (Scale != 0) {
        result.push_back('.');
        result.append(representation.substr(integer_end + 1, Scale));
    }
    return result;
}

}  // namespace storages::odbc

USERVER_NAMESPACE_END
