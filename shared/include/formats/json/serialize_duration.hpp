#pragma once

#include <chrono>

#include <formats/json/value_builder.hpp>

namespace formats::serialize {

// Declare Serialize() as static as multiple translation units might declare
// different implementations of std::chrono::duration<> serializations.
template <class Rep, class Period>
static json::Value Serialize(std::chrono::duration<Rep, Period> duration,
                             To<::formats::json::Value>) {
  return json::ValueBuilder(duration.count()).ExtractValue();
}

}  // namespace formats::serialize
