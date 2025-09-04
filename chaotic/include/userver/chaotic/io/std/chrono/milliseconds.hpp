#pragma once

#include <chrono>
#include <string>
#include <string_view>

#include <userver/chaotic/convert.hpp>
#include <userver/utils/numeric_cast.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic::convert {

template <typename T>
T Convert(std::chrono::milliseconds value, chaotic::convert::To<T>) {
    return utils::numeric_cast<T>(value.count());
}

std::chrono::milliseconds Convert(const std::string& str, chaotic::convert::To<std::chrono::milliseconds>);

std::chrono::milliseconds Convert(std::string_view str, chaotic::convert::To<std::chrono::milliseconds>);

}  // namespace chaotic::convert

USERVER_NAMESPACE_END
