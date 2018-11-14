#pragma once

#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>

#include <boost/numeric/conversion/cast.hpp>
#include <boost/optional.hpp>

#include <storages/mongo/mongo.hpp>
#include <utils/meta.hpp>
#include <yaml_config/value.hpp>

namespace taxi_config {

class DocsMap {
 public:
  storages::mongo::DocumentElement Get(const std::string& name) const;
  void Set(std::string name, storages::mongo::DocumentValue);
  void Parse(const std::string& json);
  size_t Size() const;

 private:
  std::unordered_map<std::string, storages::mongo::DocumentValue> docs_;
};

namespace impl {

template <class T>
static constexpr bool IsSpecialized =
    !meta::is_bool<T>::value &&
    (std::is_arithmetic<T>::value || meta::is_duration<T>::value ||
     meta::is_vector<T>::value || meta::is_map<T>::value);

template <typename T>
typename std::enable_if_t<!IsSpecialized<T>, T> MongoCast(
    const storages::mongo::DocumentElement& elem);

template <class T>
typename std::enable_if_t<std::is_floating_point<T>::value, T> MongoCast(
    const storages::mongo::DocumentElement& elem) {
  return static_cast<T>(storages::mongo::ToDouble(elem));
}

template <class T>
typename std::enable_if_t<
    std::is_integral<T>::value && !meta::is_bool<T>::value, T>
MongoCast(const storages::mongo::DocumentElement& elem) {
  return boost::numeric_cast<T>(storages::mongo::ToInt64(elem));
}

template <class T>
typename std::enable_if_t<meta::is_duration<T>::value, T> MongoCast(
    const storages::mongo::DocumentElement& elem) {
  return T(MongoCast<typename T::rep>(elem));
}

template <class T>
typename std::enable_if_t<meta::is_map<T>::value, T> MongoCast(
    const storages::mongo::DocumentElement& elem) {
  T response;
  const auto& obj = storages::mongo::ToDocument(elem);
  for (const auto& elem : obj) {
    response.emplace(elem.key(), MongoCast<typename T::mapped_type>(elem));
  }
  return response;
}

template <class T>
typename std::enable_if_t<meta::is_vector<T>::value, T> MongoCast(
    const storages::mongo::DocumentElement& elem) {
  T response;
  for (const auto& subitem : storages::mongo::ToArray(elem))
    response.emplace_back(MongoCast<typename T::value_type>(subitem));

  return response;
}

template <typename Res>
Res Parse(const std::string& name, const DocsMap& mongo_docs) {
  auto const element = mongo_docs.Get(name);
  return MongoCast<Res>(element);
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
