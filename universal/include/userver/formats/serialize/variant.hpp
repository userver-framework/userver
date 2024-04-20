#pragma once

/// @file userver/formats/serialize/variant.hpp
/// @brief Serializers for std::variant
/// @ingroup userver_universal userver_formats_serialize

#include <variant>

#include <userver/formats/serialize/to.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::serialize {

template <typename Value, typename... Types>
Value Serialize(const std::variant<Types...>& value, To<Value>) {
  return std::visit(
      [](const auto& item) {
        return typename Value::Builder(item).ExtractValue();
      },
      value);
}

}  // namespace formats::serialize

USERVER_NAMESPACE_END
