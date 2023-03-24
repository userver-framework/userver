#pragma once

/// @file userver/formats/parse/common_containers.hpp
/// @brief Parsers and converters for Standard Library containers and
/// std::optional
///
/// @ingroup userver_formats_parse

#include <userver/formats/parse/to.hpp>

#include <map>
#include <optional>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

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

template <typename ArrayType, typename T, class Value, typename ExtractFunc>
ArrayType ParseArray(const Value& value, ExtractFunc&& extract_func) {
  value.CheckArrayOrNull();

  ArrayType response;
  auto inserter = std::inserter(response, response.end());
  for (const auto& subitem : value) {
    *inserter = extract_func(subitem);
    inserter++;
  }
  return response;
}

template <typename ObjectType, typename T, class Value, typename ExtractFunc>
ObjectType ParseObject(const Value& value, ExtractFunc&& extract_func) {
  value.CheckObjectOrNull();

  ObjectType result;

  for (auto it = value.begin(); it != value.end(); ++it)
    result.emplace(it.GetName(), extract_func(*it));

  return result;
}

}  // namespace impl

template <class Value, typename T, typename Hash = std::hash<T>>
std::unordered_set<T, Hash> Parse(const Value& value,
                                  To<std::unordered_set<T, Hash>>) {
  return impl::ParseArray<std::unordered_set<T, Hash>, T>(
      value, &impl::AsExtractor<T, Value>);
}

template <class Value, typename T>
std::set<T> Parse(const Value& value, To<std::set<T>>) {
  return impl::ParseArray<std::set<T>, T>(value, &impl::AsExtractor<T, Value>);
}

template <class Value, typename T>
std::vector<T> Parse(const Value& value, To<std::vector<T>>) {
  return impl::ParseArray<std::vector<T>, T>(value,
                                             &impl::AsExtractor<T, Value>);
}

template <class Value, typename T, class Hash = std::hash<std::string>,
          class KeyEqual = std::equal_to<std::string>>
std::unordered_map<std::string, T, Hash, KeyEqual> Parse(
    const Value& value,
    To<std::unordered_map<std::string, T, Hash, KeyEqual>>) {
  return impl::ParseObject<std::unordered_map<std::string, T, Hash, KeyEqual>,
                           T>(value, &impl::AsExtractor<T, Value>);
}

template <class Value, typename T>
std::map<std::string, T> Parse(const Value& value,
                               To<std::map<std::string, T>>) {
  return impl::ParseObject<std::map<std::string, T>, T>(
      value, &impl::AsExtractor<T, Value>);
}

template <class Value, typename T>
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

template <class Value, typename T>
std::unordered_set<T> Convert(const Value& value, To<std::unordered_set<T>>) {
  if (value.IsMissing()) {
    return {};
  }
  return impl::ParseArray<std::unordered_set<T>, T>(
      value, &impl::ConvertToExtractor<T, Value>);
}

template <class Value, typename T>
std::set<T> Convert(const Value& value, To<std::set<T>>) {
  if (value.IsMissing()) {
    return {};
  }
  return impl::ParseArray<std::set<T>, T>(value,
                                          &impl::ConvertToExtractor<T, Value>);
}

template <class Value, typename T>
std::vector<T> Convert(const Value& value, To<std::vector<T>>) {
  if (value.IsMissing()) {
    return {};
  }
  return impl::ParseArray<std::vector<T>, T>(
      value, &impl::ConvertToExtractor<T, Value>);
}

template <class Value, typename T>
std::unordered_map<std::string, T> Convert(
    const Value& value, To<std::unordered_map<std::string, T>>) {
  if (value.IsMissing()) {
    return {};
  }
  return impl::ParseObject<std::unordered_map<std::string, T>, T>(
      value, &impl::ConvertToExtractor<T, Value>);
}

template <class Value, typename T>
std::map<std::string, T> Convert(const Value& value,
                                 To<std::map<std::string, T>>) {
  if (value.IsMissing()) {
    return {};
  }
  return impl::ParseObject<std::map<std::string, T>, T>(
      value, &impl::ConvertToExtractor<T, Value>);
}

template <class Value, typename T>
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
