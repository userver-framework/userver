#pragma once

#include <chrono>

#include <userver/chaotic/convert.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic::convert {

template <typename Period>
double Convert(const std::chrono::duration<double, Period>& value,
               chaotic::convert::To<double>) {
  return value.count();
}

}  // namespace chaotic::convert

USERVER_NAMESPACE_END
