#pragma once

/// @file formats/serialize/time_of_day.hpp
/// @brief utils::datetime::TimeOfDay to any format

#include <utils/time_of_day.hpp>

#include <formats/serialize/to.hpp>

namespace formats::serialize {

template <typename Value, typename Duration>
Value Serialize(const utils::datetime::TimeOfDay<Duration>& value, To<Value>) {
  return typename Value::Builder(fmt::format("{}", value)).ExtractValue();
}

}  // namespace formats::serialize
