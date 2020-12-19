#pragma once

/// @file formats/serialize/time_of_day.hpp
/// @brief utils::datetime::TimeOfDay to any format

#include <fmt/compile.h>
#include <fmt/format.h>

#include <utils/time_of_day.hpp>

#include <formats/serialize/to.hpp>

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
