#pragma once

// Utilitary header for chaotic for a custom type serialization/parsing support

#include <concepts>
#include <cstddef>

#include <userver/chaotic/convert/to.hpp>
#include <userver/utils/numeric_cast.hpp>

#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

namespace chaotic::convert {

template <std::integral T>
T Convert(const std::uint64_t& value, To<T>) {
    return utils::numeric_cast<T>(value);
}

template <std::integral T>
std::uint64_t Convert(const T& value, To<std::uint64_t>) {
    return utils::numeric_cast<std::uint64_t>(value);
}

}  // namespace chaotic::convert

USERVER_NAMESPACE_END
