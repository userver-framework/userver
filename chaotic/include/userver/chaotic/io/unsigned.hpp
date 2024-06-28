#pragma once

#include <cstddef>

#include <fmt/format.h>

#include <userver/chaotic/convert/to.hpp>
#include <userver/chaotic/io/std/uint32_t.hpp>
#include <userver/utils/numeric_cast.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic::convert {

template <typename T>
std::enable_if_t<
    std::is_integral_v<T> && !std::is_same_v<unsigned, std::uint32_t>, T>
Convert(unsigned value, To<T>) {
  return utils::numeric_cast<T>(value);
}

template <typename T>
std::enable_if_t<
    std::is_integral_v<T> && !std::is_same_v<unsigned, std::uint32_t>, unsigned>
Convert(T&& value, To<unsigned>) {
  return utils::numeric_cast<unsigned>(value);
}

}  // namespace chaotic::convert

USERVER_NAMESPACE_END
