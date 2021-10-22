#pragma once

/// @file userver/formats/json/serialize_duration.hpp
/// @brief Serializers for std::chrono::duration types.
/// @ingroup userver_formats_serialize

#include <chrono>

#include <userver/formats/json/string_builder_fwd.hpp>
#include <userver/formats/json/value_builder.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::serialize {

// Declare Serialize() as static as multiple translation units might declare
// different implementations of std::chrono::duration<> serializations.
template <class Rep, class Period>
static json::Value Serialize(std::chrono::duration<Rep, Period> duration,
                             To<formats::json::Value>) {
  return json::ValueBuilder(duration.count()).ExtractValue();
}

// Declare WriteToStream() as static as multiple translation units might declare
// different implementations of std::chrono::duration<> serializations.
template <class Rep, class Period>
static void WriteToStream(std::chrono::duration<Rep, Period> duration,
                          formats::json::StringBuilder& sw) {
  WriteToStream(duration.count(), sw);
}

}  // namespace formats::serialize

USERVER_NAMESPACE_END
