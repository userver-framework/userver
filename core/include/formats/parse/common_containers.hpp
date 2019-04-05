#pragma once

#include <formats/parse/to.hpp>

#include <utils/meta.hpp>

#include <boost/optional.hpp>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

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

template <class Value, typename T>
std::unordered_set<T> Parse(const Value& value, To<std::unordered_set<T>>) {
  return impl::ParseArray<std::unordered_set<T>, T>(
      value, &impl::AsExtractor<T, Value>);
}

template <class Value, typename T>
std::set<T> Parse(const Value& value, To<std::set<T>>) {
  return impl::ParseArray<std::set<T>, T>(value, &impl::AsExtractor<T, Value>);
}

template <class Value, typename T>
std::enable_if_t<meta::is_vector<T>::value, T> Parse(const Value& value,
                                                     To<T>) {
  return impl::ParseArray<T, typename T::value_type>(
      value, &impl::AsExtractor<typename T::value_type, Value>);
}

template <class Value, typename T>
std::unordered_map<std::string, T> Parse(
    const Value& value, To<std::unordered_map<std::string, T>>) {
  return impl::ParseObject<std::unordered_map<std::string, T>, T>(
      value, &impl::AsExtractor<T, Value>);
}

template <class Value, typename T>
std::map<std::string, T> Parse(const Value& value,
                               To<std::map<std::string, T>>) {
  return impl::ParseObject<std::map<std::string, T>, T>(
      value, &impl::AsExtractor<T, Value>);
}

template <class Value, typename T>
boost::optional<T> Parse(const Value& value, To<boost::optional<T>>) {
  if (value.IsMissing()) {
    return boost::none;
  }
  return value.template As<T>();
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
std::enable_if_t<meta::is_vector<T>::value, T> Convert(const Value& value,
                                                       To<T>) {
  if (value.IsMissing()) {
    return {};
  }
  return impl::ParseArray<T, typename T::value_type>(
      value, &impl::ConvertToExtractor<typename T::value_type, Value>);
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
boost::optional<T> Convert(const Value& value, To<boost::optional<T>>) {
  if (value.IsMissing()) {
    return boost::none;
  }
  return value.template ConvertTo<T>();
}

}  // namespace formats::parse
