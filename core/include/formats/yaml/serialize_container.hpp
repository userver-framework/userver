#pragma once

#include <formats/yaml/value.hpp>

#include <boost/optional.hpp>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace formats {
namespace yaml {

namespace impl {

template <typename ArrayType, typename T>
ArrayType ParseYamlArray(const formats::yaml::Value& value) {
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
ObjectType ParseYamlObject(const formats::yaml::Value& value) {
  value.CheckObjectOrNull();

  ObjectType result;

  for (auto it = value.begin(); it != value.end(); ++it)
    result.emplace(it.GetName(), it->As<T>());

  return result;
}

}  // namespace impl

template <typename T>
std::unordered_set<T> ParseYaml(const formats::yaml::Value& value,
                                const std::unordered_set<T>*) {
  return impl::ParseYamlArray<std::unordered_set<T>, T>(value);
}

template <typename T>
std::set<T> ParseYaml(const formats::yaml::Value& value, const std::set<T>*) {
  return impl::ParseYamlArray<std::set<T>, T>(value);
}

template <typename T>
std::vector<T> ParseYaml(const formats::yaml::Value& value,
                         const std::vector<T>*) {
  return impl::ParseYamlArray<std::vector<T>, T>(value);
}

template <typename T>
std::unordered_map<std::string, T> ParseYaml(
    const formats::yaml::Value& value,
    const std::unordered_map<std::string, T>*) {
  return impl::ParseYamlObject<std::unordered_map<std::string, T>, T>(value);
}

template <typename T>
std::map<std::string, T> ParseYaml(const formats::yaml::Value& value,
                                   const std::map<std::string, T>*) {
  return impl::ParseYamlObject<std::map<std::string, T>, T>(value);
}

template <typename T>
boost::optional<T> ParseYaml(const formats::yaml::Value& value,
                             const boost::optional<T>*) {
  if (value.IsMissing())
    return boost::none;
  else
    return value.As<T>();
}

}  // namespace yaml
}  // namespace formats
