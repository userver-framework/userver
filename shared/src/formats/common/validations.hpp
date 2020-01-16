#pragma once
#include <cmath>

namespace formats::common {

/// common restrictions to floating type values constructable via ValueBuilders
template <typename ExceptionType, typename Float>
Float validate_float(Float value) {
  if (std::isnan(value)) {
    throw ExceptionType("nan floating point values are forbidden");
  }
  return value;
}

}  // namespace formats::common
