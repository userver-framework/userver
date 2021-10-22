#pragma once

/// @file userver/formats/serialize/boost_optional.hpp
/// @brief Serializers for boost::optional
/// @ingroup userver_formats_serialize

#include <boost/optional.hpp>

#include <userver/formats/common/type.hpp>
#include <userver/formats/serialize/to.hpp>

USERVER_NAMESPACE_BEGIN

/// Common serializers
namespace formats::serialize {

template <typename T, typename Value>
Value Serialize(const boost::optional<T>& value, To<Value>) {
  if (!value) return {};

  return typename Value::Builder(*value).ExtractValue();
}

}  // namespace formats::serialize

USERVER_NAMESPACE_END
