#pragma once

#include <any>
#include <stdexcept>
#include <typeindex>
#include <typeinfo>
#include <vector>

#include <taxi_config/value.hpp>

namespace taxi_config {

namespace impl {

using Factory = std::any (*)(const DocsMap&);

template <typename T>
std::any FactoryFor(const DocsMap& map) {
  return std::any{T(map)};
}

[[noreturn]] void WrapGetError(const std::exception& ex, std::type_index type);

}  // namespace impl

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
  /// @warning Must not be used explicitly. Use `MakeTaxiConfigPtr` instead!
  static BaseConfig Parse(const DocsMap& docs_map);

  BaseConfig(BaseConfig&&) noexcept = default;
  BaseConfig& operator=(BaseConfig&&) noexcept = default;

  template <typename T>
  const T& Get() const;

  template <typename T>
  static void Unregister();

  template <typename T>
  static bool IsRegistered();

 private:
  using ConfigId = std::size_t;

  explicit BaseConfig(const DocsMap& docs_map);

  const std::any& Get(ConfigId id) const;

  static ConfigId Register(impl::Factory factory);

  static void Unregister(ConfigId id);

  static bool IsRegistered(ConfigId id);

  // Automatically registers all used config types at startup and assigns them
  // sequential ids
  template <typename T>
  static inline const ConfigId kConfigId = Register(&impl::FactoryFor<T>);

  std::vector<std::any> user_configs_;
};

using Config = BaseConfig<struct FullConfigTag>;
using BootstrapConfig = BaseConfig<struct BootstrapConfigTag>;

/// @cond
extern template class BaseConfig<FullConfigTag>;
extern template class BaseConfig<BootstrapConfigTag>;

template <typename ConfigTag>
template <typename T>
const T& BaseConfig<ConfigTag>::Get() const {
  try {
    return std::any_cast<const T&>(Get(kConfigId<T>));
  } catch (const std::exception& ex) {
    impl::WrapGetError(ex, typeid(T));
  }
}

template <typename ConfigTag>
template <typename T>
void BaseConfig<ConfigTag>::Unregister() {
  Unregister(kConfigId<T>);
}

template <typename ConfigTag>
template <typename T>
bool BaseConfig<ConfigTag>::IsRegistered() {
  return IsRegistered(kConfigId<T>);
}
/// @endcond

}  // namespace taxi_config
