#pragma once

#include <formats/json/value.hpp>

#include <boost/optional.hpp>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace formats {
namespace json {

namespace impl {

template <typename ArrayType, typename T>
ArrayType ParseJsonArray(const formats::json::Value& value) {
  value.CheckArrayOrNull();

  ArrayType response;
  auto inserter = std::inserter(response, response.end());
  for (const auto& subitem : value) {
    *inserter = subitem.As<T>();
    inserter++;
  }
  return response;
}

template <typename ObjectType, typename T>
ObjectType ParseJsonObject(const formats::json::Value& value) {
  value.CheckObjectOrNull();

  ObjectType result;

  for (auto it = value.begin(); it != value.end(); ++it)
    result.emplace(it.GetName(), it->As<T>());

  return result;
}

}  // namespace impl

template <typename T>
std::unordered_set<T> ParseJson(const formats::json::Value& value,
                                const std::unordered_set<T>*) {
  return impl::ParseJsonArray<std::unordered_set<T>, T>(value);
}

template <typename T>
std::set<T> ParseJson(const formats::json::Value& value, const std::set<T>*) {
  return impl::ParseJsonArray<std::set<T>, T>(value);
}

template <typename T>
std::vector<T> ParseJson(const formats::json::Value& value,
                         const std::vector<T>*) {
  return impl::ParseJsonArray<std::vector<T>, T>(value);
}

template <typename T>
std::unordered_map<std::string, T> ParseJson(
    const formats::json::Value& value,
    const std::unordered_map<std::string, T>*) {
  return impl::ParseJsonObject<std::unordered_map<std::string, T>, T>(value);
}

template <typename T>
std::map<std::string, T> ParseJson(const formats::json::Value& value,
                                   const std::map<std::string, T>*) {
  return impl::ParseJsonObject<std::map<std::string, T>, T>(value);
}

template <typename T>
boost::optional<T> ParseJson(const formats::json::Value& value,
                             const boost::optional<T>*) {
  if (value.IsMissing())
    return boost::none;
  else
    return value.As<T>();
}

}  // namespace json
}  // namespace formats
