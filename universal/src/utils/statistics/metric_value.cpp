#include <userver/utils/statistics/metric_value.hpp>

#include <cmath>
#include <limits>
#include <type_traits>
#include <utility>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {
namespace {

template <typename Integer>
bool IsWithinIntegerLimits(double real) {
    static_assert(std::is_integral_v<Integer>);
    if (!std::isfinite(real)) {
        return false;
    }
    constexpr auto kMin = static_cast<double>(std::numeric_limits<Integer>::min());
    constexpr auto kMax = static_cast<double>(std::numeric_limits<Integer>::max());
    // kMax is 2^digits when double cannot represent max. A wider double also has
    // values between max and 2^digits, and those do not fit either.
    const double two_digits = std::ldexp(1.0, std::numeric_limits<Integer>::digits);
    return real >= kMin && real <= kMax && real < two_digits;
}

template <typename Integer>
bool FloatEqualsInteger(double real, Integer integer) {
    if (!IsWithinIntegerLimits<Integer>(real)) {
        return false;
    }
    const auto converted = static_cast<Integer>(real);
    // A fractional value is within the limits but is not that integer.
    if (static_cast<double>(converted) != real) {
        return false;
    }
    return converted == integer;
}

template <typename Lhs, typename Rhs>
bool ValuesEqual(Lhs lhs, Rhs rhs) {
    if constexpr (std::is_same_v<Lhs, Rhs>) {
        return lhs == rhs;
    } else if constexpr (std::is_same_v<Lhs, Rate>) {
        return ValuesEqual(lhs.value, rhs);
    } else if constexpr (std::is_same_v<Rhs, Rate>) {
        return ValuesEqual(lhs, rhs.value);
    } else if constexpr (std::is_integral_v<Lhs> && std::is_integral_v<Rhs>) {
        return std::cmp_equal(lhs, rhs);
    } else if constexpr (std::is_same_v<Lhs, double> && std::is_integral_v<Rhs>) {
        return FloatEqualsInteger(lhs, rhs);
    } else if constexpr (std::is_integral_v<Lhs> && std::is_same_v<Rhs, double>) {
        return FloatEqualsInteger(rhs, lhs);
    } else {
        return false;
    }
}

}  // namespace

bool MetricValue::EqualsNumerically(const MetricValue& other) const noexcept {
    return Visit([&other](const auto lhs_value) {
        return other.Visit([lhs_value](const auto rhs_value) { return ValuesEqual(lhs_value, rhs_value); });
    });
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
