#pragma once

/// @file userver/formats/parse/common_containers.hpp
/// @brief Parsers and converters for Standard Library containers and
/// std::optional
///
/// @ingroup userver_formats_parse

#include <optional>
#include <type_traits>

#include <userver/formats/common/meta.hpp>
#include <userver/formats/parse/to.hpp>
#include <userver/utils/meta.hpp>

namespace boost::uuids {
struct uuid;
}

USERVER_NAMESPACE_BEGIN

namespace formats::parse {

namespace impl {

template <typename T, class Value>
inline T AsExtractor(const Value& value) {
  return value.template As<T>();
}

template <typename T, class Value>
inline T ConvertToExtractor(const Value& value) {
  return value.template ConvertTo<T>();
}

template <typename ArrayType, class Value, typename ExtractFunc>
ArrayType ParseArray(const Value& value, ExtractFunc&& extract_func) {
  value.CheckArrayOrNull();
  ArrayType response;
  auto inserter = std::inserter(response, response.end());

  for (const auto& subitem : value) {
    *inserter = extract_func(subitem);
    ++inserter;
  }

  return response;
}

template <typename ObjectType, class Value, typename ExtractFunc>
ObjectType ParseObject(const Value& value, ExtractFunc&& extract_func) {
  value.CheckObjectOrNull();
  ObjectType result;

  for (auto it = value.begin(); it != value.end(); ++it) {
    result.emplace(it.GetName(), extract_func(*it));
  }

  return result;
}

}  // namespace impl

template <typename T, typename Value>
std::enable_if_t<common::kIsFormatValue<Value> && meta::kIsRange<T> &&
                     !meta::kIsMap<T> && !std::is_same_v<T, boost::uuids::uuid>,
                 T>
Parse(const Value& value, To<T>) {
  return impl::ParseArray<T>(
      value, &impl::AsExtractor<meta::RangeValueType<T>, Value>);
}

template <typename T, typename Value>
std::enable_if_t<common::kIsFormatValue<Value> && meta::kIsMap<T>, T> Parse(
    const Value& value, To<T>) {
  return impl::ParseObject<T>(
      value, &impl::AsExtractor<typename T::mapped_type, Value>);
}

template <typename T, typename Value>
std::optional<T> Parse(const Value& value, To<std::optional<T>>) {
  if (value.IsMissing() || value.IsNull()) {
    return std::nullopt;
  }
  return value.template As<T>();
}

template <class Value>
std::optional<std::nullptr_t> Parse(const Value&,
                                    To<std::optional<std::nullptr_t>>) {
  static_assert(!sizeof(Value),
                "optional<nullptr_t> is forbidden, check IsNull() instead");
  return nullptr;
}

template <typename T, typename Value>
std::enable_if_t<meta::kIsRange<T> && !meta::kIsMap<T> &&
                     !std::is_same_v<T, boost::uuids::uuid>,
                 T>
Convert(const Value& value, To<T>) {
  if (value.IsMissing()) {
    return {};
  }
  return impl::ParseArray<T>(
      value, &impl::ConvertToExtractor<meta::RangeValueType<T>, Value>);
}

template <typename T, typename Value>
std::enable_if_t<meta::kIsMap<T>, T> Convert(const Value& value, To<T>) {
  if (value.IsMissing()) {
    return {};
  }
  return impl::ParseObject<T>(
      value, &impl::ConvertToExtractor<typename T::mapped_type, Value>);
}

template <typename T, typename Value>
std::optional<T> Convert(const Value& value, To<std::optional<T>>) {
  if (value.IsMissing() || value.IsNull()) {
    return std::nullopt;
  }
  return value.template ConvertTo<T>();
}

template <class Value>
std::optional<std::nullptr_t> Convert(const Value&,
                                      To<std::optional<std::nullptr_t>>) {
  static_assert(!sizeof(Value),
                "optional<nullptr_t> is forbidden, check IsNull() instead");
  return nullptr;
}

}  // namespace formats::parse

USERVER_NAMESPACE_END
