#pragma once

#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>

#include <boost/numeric/conversion/cast.hpp>
#include <boost/optional.hpp>

#include <engine/mutex.hpp>
#include <formats/json/value.hpp>
#include <utils/meta.hpp>
#include <yaml_config/value.hpp>

namespace taxi_config {

class DocsMap {
 public:
  /* Returns config item or throws an exception if key is missing */
  formats::json::Value Get(const std::string& name) const;

  void Set(std::string name, formats::json::Value);
  void Parse(const std::string& json, bool empty_ok);
  size_t Size() const;

  auto GetMap() const { return docs_; }
  void MergeFromOther(DocsMap&& other);
  std::vector<std::string> GetRequestedNames() const;

  std::string AsJsonString() const;

 private:
  std::unordered_map<std::string, formats::json::Value> docs_;
  mutable std::unordered_set<std::string> requested_names_;
};

namespace impl {

template <typename Res>
Res Parse(const std::string& name, const DocsMap& mongo_docs) {
  auto const element = mongo_docs.Get(name);
  return element.As<Res>();
}

}  // namespace impl

template <typename T>
class Value {
 public:
  Value(const std::string& name, const DocsMap& mongo_docs)
      : value_(impl::Parse<T>(name, mongo_docs)) {}

  operator const T&() const { return value_; }

  const T& Get() const { return value_; }
  const T* operator->() const { return &value_; }

 private:
  T value_;
};

}  // namespace taxi_config

template <typename T>
std::unordered_map<std::string, T> ParseJson(
    const formats::json::Value& elem, std::unordered_map<std::string, T>*) {
  std::unordered_map<std::string, T> response;
  for (auto it = elem.begin(); it != elem.end(); ++it) {
    response.emplace(it.GetName(), it->As<T>());
  }
  return response;
}

template <typename T>
std::map<std::string, T> ParseJson(const formats::json::Value& elem,
                                   std::map<std::string, T>*) {
  std::map<std::string, T> response;
  for (auto it = elem.begin(); it != elem.end(); ++it) {
    response.emplace(it.GetName(), it->As<T>());
  }
  return response;
}

template <typename T>
std::vector<T> ParseJson(const formats::json::Value& elem, std::vector<T>*) {
  std::vector<T> response;
  for (const auto& item : elem)
    response.emplace_back(item.As<typename T::value_type>());
  return response;
}

namespace formats::json {
std::unordered_set<std::string> ParseJson(const formats::json::Value& elem,
                                          std::unordered_set<std::string>*);
}
