#pragma once

/// @file userver/formats/serialize/time_of_day.hpp
/// @brief utils::datetime::TimeOfDay to any format
/// @ingroup userver_formats_serialize

#include <fmt/format.h>

#include <userver/utils/time_of_day.hpp>

#include <userver/formats/serialize/to.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::serialize {

template <typename Value, typename Duration>
Value Serialize(const utils::datetime::TimeOfDay<Duration>& value, To<Value>) {
  return typename Value::Builder(fmt::to_string(value)).ExtractValue();
}

template <typename Value, typename Duration, typename StringBuilder>
void WriteToStream(const utils::datetime::TimeOfDay<Duration>& value,
                   StringBuilder& sw) {
  WriteToStream(fmt::to_string(value), sw);
}

}  // namespace formats::serialize

USERVER_NAMESPACE_END
