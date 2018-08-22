#pragma once

#include <typeindex>

#include <boost/any.hpp>

#include "value.hpp"

namespace taxi_config {

class Config;

template <typename T>
class ConfigModule {
 public:
  static const T& Get(const Config& config);
  static boost::any Factory(const DocsMap& docs_map) { return T(docs_map); }

 private:
  static std::type_index type_;
};

class Config {
public:
  Config(const DocsMap& docs_map);

  Config(Config&&);


  ~Config();

  template <typename T>
  static std::type_index Register(
      std::function<boost::any(const DocsMap&)>&& factory) {
    Register(typeid(T), std::move(factory));
    return typeid(T);
  }

  template <typename T>
  const T& Get() const {
    return ConfigModule<T>::Get(*this);
  }

 private:
  static void Register(const std::type_info& type,
                       std::function<boost::any(const DocsMap&)>&& factory);
  const boost::any& Get(const std::type_index& type) const;

  std::unordered_map<std::type_index, boost::any> extra_configs_;

  template <typename T>
  friend class ConfigModule;
};

template <typename T>
const T& ConfigModule<T>::Get(const Config& config) {
  return boost::any_cast<const T&>(config.Get(type_));
}

template <typename T>
std::type_index ConfigModule<T>::type_ =
    Config::Register<T>(&ConfigModule<T>::Factory);

}
