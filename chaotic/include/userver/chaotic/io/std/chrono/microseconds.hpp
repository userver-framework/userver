#pragma once

#include <chrono>

#include <userver/chaotic/convert.hpp>
#include <userver/utils/numeric_cast.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic::convert {

template <typename T>
T Convert(std::chrono::microseconds value, chaotic::convert::To<T>) {
  return utils::numeric_cast<T>(value.count());
}

}  // namespace chaotic::convert

USERVER_NAMESPACE_END
