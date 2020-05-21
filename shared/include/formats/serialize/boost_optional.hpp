#pragma once

/// @file formats/serialize/common_containers.hpp
/// @brief Serializers for boost::optional

#include <boost/optional.hpp>

#include <formats/common/type.hpp>
#include <formats/serialize/to.hpp>

/// Common serializers
namespace formats::serialize {

template <typename T, typename Value>
Value Serialize(const boost::optional<T>& value, To<Value>) {
  if (!value) return {};

  return typename Value::Builder(*value).ExtractValue();
}

}  // namespace formats::serialize
