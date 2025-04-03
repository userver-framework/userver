#pragma once

/// @file userver/utils/numeric_cast.hpp
/// @brief @copybrief utils::numeric_cast

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
template <typename To, typename From>
constexpr To numeric_cast(From input) {
    static_assert(std::is_integral_v<From>);
    static_assert(std::is_integral_v<To>);
    using FromLimits = std::numeric_limits<From>;
    using ToLimits = std::numeric_limits<To>;

    std::string_view overflow_type{};

    if constexpr (ToLimits::digits < FromLimits::digits) {
        if (input > static_cast<From>(ToLimits::max())) {
            overflow_type = "positive";
        }
    }

    // signed -> signed: loss if narrowing
    // signed -> unsigned: loss
    if constexpr (FromLimits::is_signed && (!ToLimits::is_signed || ToLimits::digits < FromLimits::digits)) {
        if (input < static_cast<From>(ToLimits::lowest())) {
            overflow_type = "negative";
        }
    }

    if (!overflow_type.empty()) {
        throw std::runtime_error(fmt::format(
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
