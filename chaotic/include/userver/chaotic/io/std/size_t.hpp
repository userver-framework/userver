#pragma once

#include <cstddef>

#include <userver/chaotic/convert/to.hpp>
#include <userver/chaotic/io/std/uint64_t.hpp>
#include <userver/utils/numeric_cast.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic::convert {

template <typename T>
std::enable_if_t<
    std::is_integral_v<T> && !std::is_same_v<std::size_t, std::uint64_t>, T>
Convert(const std::size_t& value, To<T>) {
  return utils::numeric_cast<T>(value);
}

template <typename T>
std::enable_if_t<std::is_integral_v<T> &&
                     !std::is_same_v<std::size_t, std::uint64_t>,
                 std::size_t>
Convert(T&& value, To<std::size_t>) {
  return utils::numeric_cast<std::size_t>(value);
}

}  // namespace chaotic::convert

USERVER_NAMESPACE_END
