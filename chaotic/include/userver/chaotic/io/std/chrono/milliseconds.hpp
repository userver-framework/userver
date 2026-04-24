#pragma once

// Utilitary header for chaotic for a custom type serialization/parsing support

#include <chrono>
#include <string>
#include <string_view>

#include <userver/chaotic/convert.hpp>
#include <userver/utils/numeric_cast.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic::convert {

template <typename T>
requires std::is_integral_v<T> || std::is_floating_point_v<T>
T Convert(std::chrono::milliseconds value, chaotic::convert::To<T>) {
    return utils::numeric_cast<T>(value.count());
}

std::string Convert(const std::chrono::milliseconds& value, chaotic::convert::To<std::string>);

std::chrono::milliseconds Convert(const std::string& str, chaotic::convert::To<std::chrono::milliseconds>);

std::chrono::milliseconds Convert(std::string_view str, chaotic::convert::To<std::chrono::milliseconds>);

}  // namespace chaotic::convert

USERVER_NAMESPACE_END
