#pragma once

#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>

#include <mongo/bson/bson.h>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/optional.hpp>

#include <json_config/value.hpp>
#include <storages/mongo/mongo.hpp>
#include <storages/mongo/wrappers.hpp>
#include <utils/meta.hpp>

namespace taxi_config {

class DocsMap {
 public:
  mongo::BSONElement Get(const std::string& name) const;
  void Set(std::string name, mongo::BSONObj);
  void Parse(const std::string& json);

 private:
  std::unordered_map<std::string, mongo::BSONObj> docs_;
};

namespace impl {

template <class T>
static constexpr bool IsSpecialized =
    !meta::is_bool<T>::value &&
    (std::is_arithmetic<T>::value || meta::is_duration<T>::value ||
     meta::is_vector<T>::value || meta::is_map<T>::value);

template <typename T>
typename std::enable_if_t<!IsSpecialized<T>, T> MongoCast(
    const ::mongo::BSONElement& elem);

template <class T>
typename std::enable_if_t<std::is_floating_point<T>::value, T> MongoCast(
    const ::mongo::BSONElement& elem) {
  return static_cast<T>(storages::mongo::ToDouble(elem));
}

template <class T>
typename std::enable_if_t<std::is_integral<T>::value &&
                              std::is_signed<T>::value &&
                              !meta::is_bool<T>::value,
                          T>
MongoCast(const ::mongo::BSONElement& elem) {
  return boost::numeric_cast<T>(storages::mongo::ToInt(elem));
}

template <class T>
typename std::enable_if_t<std::is_integral<T>::value &&
                              std::is_unsigned<T>::value &&
                              !meta::is_bool<T>::value,
                          T>
MongoCast(const ::mongo::BSONElement& elem) {
  return boost::numeric_cast<T>(storages::mongo::ToUint(elem));
}

template <class T>
typename std::enable_if_t<meta::is_duration<T>::value, T> MongoCast(
    const ::mongo::BSONElement& elem) {
  return T(MongoCast<T::rep>(elem));
}

template <class T>
typename std::enable_if_t<meta::is_map<T>::value, T> MongoCast(
    const ::mongo::BSONElement& elem) {
  T response;
  const auto& obj = storages::mongo::ToDocument(elem);
  for (auto i = obj.begin(); i.more(); ++i) {
    const auto& e = *i;
    response.emplace(e.fieldName(), MongoCast<typename T::mapped_type>(e));
  }
  return response;
}

template <class T>
typename std::enable_if_t<meta::is_vector<T>::value, T> MongoCast(
    const ::mongo::BSONElement& elem) {
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
