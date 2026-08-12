#pragma once

// Utilitary header for chaotic for a custom type serialization/parsing support

#include <chrono>

#include <userver/chaotic/convert.hpp>
#include <userver/utils/numeric_cast.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic::convert {

template <typename T>
T Convert(std::chrono::years value, chaotic::convert::To<T>) {
    return utils::numeric_cast<T>(value.count());
}

}  // namespace chaotic::convert

USERVER_NAMESPACE_END
