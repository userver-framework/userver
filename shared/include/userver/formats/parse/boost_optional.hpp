#pragma once

/// @file userver/formats/parse/boost_optional.hpp
/// @brief Parsers and converters for boost::optional
/// @ingroup userver_formats_parse

#include <userver/formats/parse/to.hpp>

#include <boost/optional.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::parse {

template <class Value, typename T>
boost::optional<T> Parse(const Value& value, To<boost::optional<T>>) {
  if (value.IsMissing() || value.IsNull()) {
    return boost::none;
  }
  return value.template As<T>();
}

template <class Value>
boost::optional<std::nullptr_t> Parse(const Value&,
                                      To<boost::optional<std::nullptr_t>>) {
  static_assert(!sizeof(Value),
                "optional<nullptr_t> is forbidden, check IsNull() instead");
  return nullptr;
}

template <class Value, typename T>
boost::optional<T> Convert(const Value& value, To<boost::optional<T>>) {
  if (value.IsMissing() || value.IsNull()) {
    return boost::none;
  }
  return value.template ConvertTo<T>();
}

template <class Value>
boost::optional<std::nullptr_t> Convert(const Value&,
                                        To<boost::optional<std::nullptr_t>>) {
  static_assert(!sizeof(Value),
                "optional<nullptr_t> is forbidden, check IsNull() instead");
  return nullptr;
}

}  // namespace formats::parse

USERVER_NAMESPACE_END
