#pragma once

#include <userver/formats/serialize/to.hpp>
#include <userver/utils/statistics/rate.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

template <typename ValueType>
ValueType Serialize(Rate rate, formats::serialize::To<ValueType>) {
  using ValueBuilder = typename ValueType::Builder;
  return ValueBuilder{rate.value}.ExtractValue();
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
