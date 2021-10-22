#pragma once

/// @file userver/formats/json/serialize_variant.hpp
/// @brief Serializers for std::variant
/// @ingroup userver_formats_serialize

#include <variant>

#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/serialize/to.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::serialize {

template <typename... Types>
formats::json::Value Serialize(const std::variant<Types...>& value,
                               To<formats::json::Value>) {
  return std::visit(
      [](const auto& item) {
        return formats::json::ValueBuilder(item).ExtractValue();
      },
      value);
}

}  // namespace formats::serialize

USERVER_NAMESPACE_END
