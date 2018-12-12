#pragma once

#include <typeindex>

#include <boost/any.hpp>

#include "value.hpp"

namespace taxi_config {

template <typename ConfigTag>
class BaseConfig;

template <typename ConfigTag, typename T>
class ConfigModule {
 public:
  static const T& Get(const BaseConfig<ConfigTag>& config);
  static boost::any Factory(const DocsMap& docs_map) { return T(docs_map); }

 private:
  static std::type_index type_;
};

template <typename ConfigTag>
class BaseConfig {
 public:
  BaseConfig(const DocsMap& docs_map);

  BaseConfig(BaseConfig&&) = default;

  ~BaseConfig() = default;

  template <typename T>
  static std::type_index Register(
      std::function<boost::any(const DocsMap&)>&& factory) {
    DoRegister(typeid(T), std::move(factory));
    return typeid(T);
  }

  template <typename T>
  const T& Get() const {
    return ConfigModule<ConfigTag, T>::Get(*this);
  }

  static void DoRegister(const std::type_info& type,
                         std::function<boost::any(const DocsMap&)>&& factory);

 private:
  const boost::any& Get(const std::type_index& type) const;

  std::unordered_map<std::type_index, boost::any> extra_configs_;

  template <typename U, typename T>
  friend class ConfigModule;
};

struct FullConfigTag;
struct BootstrapConfigTag;

extern template class BaseConfig<FullConfigTag>;
extern template class BaseConfig<BootstrapConfigTag>;

using Config = BaseConfig<FullConfigTag>;
using BootstrapConfig = BaseConfig<BootstrapConfigTag>;

template <typename ConfigTag, typename T>
const T& ConfigModule<ConfigTag, T>::Get(const BaseConfig<ConfigTag>& config) {
  return boost::any_cast<const T&>(config.Get(type_));
}

template <typename ConfigTag, typename T>
std::type_index ConfigModule<ConfigTag, T>::type_ =
    BaseConfig<ConfigTag>::template Register<T>(
        &ConfigModule<ConfigTag, T>::Factory);

}  // namespace taxi_config
