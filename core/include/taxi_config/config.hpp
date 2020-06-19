#pragma once

#include <any>
#include <functional>
#include <typeindex>

#include "value.hpp"

namespace taxi_config {

template <typename ConfigTag>
class BaseConfig;

template <typename ConfigTag, typename T>
class ConfigModule final {
 public:
  static const T& Get(const BaseConfig<ConfigTag>& config);
  static std::any Factory(const DocsMap& docs_map) { return T(docs_map); }
  static void Unregister();

  static bool IsRegistered();

 private:
  static std::type_index type_;
};

template <typename ConfigTag>
class BaseConfig final {
 public:
  explicit BaseConfig(const DocsMap& docs_map);

  BaseConfig(BaseConfig&&) noexcept = default;

  ~BaseConfig() = default;

  template <typename T>
  static std::type_index Register(
      std::function<std::any(const DocsMap&)>&& factory) {
    DoRegister(typeid(T), std::move(factory));
    return typeid(T);
  }

  template <typename T>
  const T& Get() const {
    return ConfigModule<ConfigTag, T>::Get(*this);
  }

  static void DoRegister(const std::type_info& type,
                         std::function<std::any(const DocsMap&)>&& factory);

  static void Unregister(const std::type_info& type);

  static bool IsRegistered(const std::type_info& type);

 private:
  const std::any& Get(const std::type_index& type) const;

  std::unordered_map<std::type_index, std::any> extra_configs_;

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
  return std::any_cast<const T&>(config.Get(type_));
}

template <typename ConfigTag, typename T>
void ConfigModule<ConfigTag, T>::Unregister() {
  BaseConfig<ConfigTag>::Unregister(typeid(T));
}

template <typename ConfigTag, typename T>
bool ConfigModule<ConfigTag, T>::IsRegistered() {
  return BaseConfig<ConfigTag>::IsRegistered(typeid(T));
}

template <typename ConfigTag, typename T>
std::type_index ConfigModule<ConfigTag, T>::type_ =
    BaseConfig<ConfigTag>::template Register<T>(
        &ConfigModule<ConfigTag, T>::Factory);

}  // namespace taxi_config
