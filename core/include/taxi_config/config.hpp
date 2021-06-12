#pragma once

#include <any>
#include <stdexcept>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <utility>
#include <vector>

#include <taxi_config/value.hpp>

namespace taxi_config {

namespace impl {

using Factory = std::any (*)(const DocsMap&);

template <typename Key>
std::any FactoryFor(const DocsMap& map) {
  return std::any{Key::Parse(map)};
}

[[noreturn]] void WrapGetError(const std::exception& ex, std::type_index type);

template <typename T>
T ParseByConstructor(const DocsMap& docs_map) {
  return T(docs_map);
}

using ConfigId = std::size_t;

ConfigId Register(Factory factory);

// Automatically registers all used config types at startup and assigns them
// sequential ids
template <typename Key>
inline const ConfigId kConfigId = Register(&FactoryFor<Key>);

}  // namespace impl

class KeyValue;

// clang-format off
/// @brief A config key is a unique identifier for a config variable
/// @snippet core/src/components/logging_configurator.cpp  LoggingConfigurator config key
// clang-format on
template <auto Parser>
struct Key final {
  static auto Parse(const DocsMap& docs_map) { return Parser(docs_map); }
};

/// Get the type of a config variable, given its key
template <typename Key>
using VariableOfKey = decltype(Key::Parse(DocsMap{}));

// clang-format off
/// @brief The storage for a snapshot of configs
///
/// When a config update comes in via new `DocsMap`, configs of all
/// the registered types are constructed and stored in `Config`. After that
/// the `DocsMap` is dropped.
///
/// Config types are automatically registered if they are accessed with `Get`
/// somewhere in the program.
///
/// ## Usage example:
/// @snippet components/component_sample_test.cpp  Sample user component runtime config source
// clang-format on
class Config final {
 public:
  /// @warning Must not be used explicitly. Use `GetDefaultSource` and
  /// `MakeDefaultStorage` instead!
  static Config Parse(const DocsMap& docs_map);

  Config(Config&&) noexcept = default;
  Config& operator=(Config&&) noexcept = default;

  template <typename Key>
  const VariableOfKey<Key>& operator[](Key) const;

  /// Deprecated, use `config[key]` instead
  template <typename T>
  const T& Get() const;

 private:
  explicit Config(const DocsMap& docs_map);

  Config(const std::vector<KeyValue>& config_variables);

  Config(const DocsMap& defaults, const std::vector<KeyValue>& overrides);

  Config(const Config& defaults, const std::vector<KeyValue>& overrides);

  const std::any& Get(impl::ConfigId id) const;

  // For the constructors
  friend class StorageMock;

  std::vector<std::any> user_configs_;
};

template <typename Key>
const VariableOfKey<Key>& Config::operator[](Key) const {
  try {
    return std::any_cast<const VariableOfKey<Key>&>(Get(impl::kConfigId<Key>));
  } catch (const std::exception& ex) {
    impl::WrapGetError(ex, typeid(VariableOfKey<Key>));
  }
}

template <typename T>
const T& Config::Get() const {
  return (*this)[Key<impl::ParseByConstructor<T>>{}];
}

}  // namespace taxi_config
