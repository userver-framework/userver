#pragma once

/// @file userver/formats/bson/serialize_duration.hpp
/// @brief Serializers for std::chrono::duration types.
/// @ingroup userver_universal userver_formats_serialize

#include <chrono>

#include <userver/formats/bson/value_builder.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::serialize {

// Declare Serialize() as static as multiple translation units might declare
// different implementations of std::chrono::duration<> serializations.
template <class Rep, class Period>
static bson::Value Serialize(std::chrono::duration<Rep, Period> duration,
                             To<formats::bson::Value>) {
  return bson::ValueBuilder(duration.count()).ExtractValue();
}

}  // namespace formats::serialize

USERVER_NAMESPACE_END
