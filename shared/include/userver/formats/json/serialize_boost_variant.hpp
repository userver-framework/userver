#pragma once

/// @file userver/formats/json/serialize_boost_variant.hpp
/// @brief Serializers for boost::variant types.
/// @ingroup userver_formats_serialize

#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/variant.hpp>

#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/serialize/to.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::serialize {

template <typename... Types>
formats::json::Value Serialize(const boost::variant<Types...>& value,
                               To<formats::json::Value>) {
  return boost::apply_visitor(
      [](const auto& item) {
        return formats::json::ValueBuilder(item).ExtractValue();
      },
      value);
}

}  // namespace formats::serialize

USERVER_NAMESPACE_END
