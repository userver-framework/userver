#pragma once

#include <any>
#include <typeindex>
#include <typeinfo>

#include <taxi_config/value.hpp>

namespace taxi_config {

/// @brief The storage for a snapshot of configs
///
/// When a config update comes in via new `DocsMap`, configs of all
/// the registered types are constructed and stored in `BaseConfig`. After that
/// the `DocsMap` is dropped.
///
/// Config types are automatically registered if they are accessed with `Get`
/// somewhere in the program.
template <typename ConfigTag>
class BaseConfig final {
 public:
  explicit BaseConfig(const DocsMap& docs_map);

  // Disable copy operations
  BaseConfig(BaseConfig&&) noexcept = default;

  template <typename T>
  const T& Get() const;

  template <typename T>
  static void Register();

  template <typename T>
  static void Unregister();

  template <typename T>
  static bool IsRegistered();

 private:
  using Factory = std::any (*)(const DocsMap&);

  static std::unordered_map<std::type_index, Factory>& ConfigFactories();

  const std::any& Get(std::type_index type) const;

  static void Register(std::type_index type, Factory factory);

  static void Unregister(std::type_index type);

  static bool IsRegistered(std::type_index type);

  std::unordered_map<std::type_index, std::any> user_configs_;
};

using Config = BaseConfig<struct FullConfigTag>;
using BootstrapConfig = BaseConfig<struct BootstrapConfigTag>;

/// @cond
extern template class BaseConfig<FullConfigTag>;
extern template class BaseConfig<BootstrapConfigTag>;

namespace impl {

// Used in `Get` to automatically call Register on all used config types
// at startup
template <typename ConfigTag, typename T>
inline const std::type_index kConfigType =
    (BaseConfig<ConfigTag>::template Register<T>(), typeid(T));

}  // namespace impl

template <typename ConfigTag>
template <typename T>
const T& BaseConfig<ConfigTag>::Get() const {
  return std::any_cast<const T&>(Get(impl::kConfigType<ConfigTag, T>));
}

template <typename ConfigTag>
template <typename T>
void BaseConfig<ConfigTag>::Register() {
  Register(typeid(T), [](const DocsMap& map) { return std::any{T(map)}; });
}

template <typename ConfigTag>
template <typename T>
void BaseConfig<ConfigTag>::Unregister() {
  Unregister(typeid(T));
}

template <typename ConfigTag>
template <typename T>
bool BaseConfig<ConfigTag>::IsRegistered() {
  return IsRegistered(typeid(T));
}
/// @endcond

}  // namespace taxi_config
