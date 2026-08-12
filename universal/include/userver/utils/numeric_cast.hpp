#pragma once

/// @file userver/utils/numeric_cast.hpp
/// @brief @copybrief utils::numeric_cast

#include <cmath>
#include <limits>
#include <stdexcept>

#include <fmt/format.h>

#include <userver/compiler/demangle.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

namespace impl {

template <typename T>
using PrintableValue = std::conditional_t<(sizeof(T) > 1), T, int>;

}

/// Detects loss of range when a numeric type is converted, and throws an
/// exception if the range cannot be preserved
///
/// ## Example usage:
///
/// @snippet utils/numeric_cast_test.cpp  Sample utils::numeric_cast usage
template <typename To, typename Exception = std::runtime_error, typename From>
constexpr To numeric_cast(From input) {  // NOLINT(readability-identifier-naming)
    constexpr bool is_integral = std::is_integral_v<From> && std::is_integral_v<To>;
    constexpr bool is_floating_point = std::is_floating_point_v<From> && std::is_floating_point_v<To>;
    static_assert(is_integral || is_floating_point);

    using FromLimits = std::numeric_limits<From>;
    using ToLimits = std::numeric_limits<To>;

    constexpr bool check_positive_overflow =
        is_integral ? ToLimits::digits < FromLimits::digits : ToLimits::max() < FromLimits::max();
    constexpr bool check_negative_overflow =
        is_integral
            ?
            // signed -> signed: possible loss if narrowing
            // signed -> unsigned: loss if negative
            FromLimits::is_signed && (!ToLimits::is_signed || ToLimits::digits < FromLimits::digits)
            : ToLimits::lowest() > FromLimits::lowest();

    std::string_view overflow_type{};

    if constexpr (check_positive_overflow) {
        if (input > static_cast<From>(ToLimits::max())) {
            if (!FromLimits::has_infinity || !std::isinf(input)) {
                overflow_type = "positive";
            }
        }
    }

    if constexpr (check_negative_overflow) {
        if (input < static_cast<From>(ToLimits::lowest())) {
            if (!FromLimits::has_infinity || !std::isinf(input)) {
                overflow_type = "negative";
            }
        }
    }

    if (!overflow_type.empty()) {
        throw Exception(fmt::format(
            "Failed to convert {} {} into {} due to {} integer overflow",
            compiler::GetTypeName<From>(),
            static_cast<impl::PrintableValue<From>>(input),
            compiler::GetTypeName<To>(),
            overflow_type
        ));
    }

    return static_cast<To>(input);
}

}  // namespace utils

USERVER_NAMESPACE_END
