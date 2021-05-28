#pragma once

#include <any>
#include <initializer_list>
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

}  // namespace impl

/// A config key is an `inline constexpr` constant used to access a config
/// variable. It must have a type of `taxi_config::Key<Parser>`, where `Parser`
/// is a function of type `(const DocsMap&) -> T`.
template <auto Parser>
struct Key final {
  static auto Parse(const DocsMap& docs_map) { return Parser(docs_map); }
};

/// Get the type of a config variable, given its key
template <typename Key>
using VariableOfKey = decltype(Key::Parse(DocsMap{}));

/// @brief The storage for a snapshot of configs
///
/// When a config update comes in via new `DocsMap`, configs of all
/// the registered types are constructed and stored in `Config`. After that
/// the `DocsMap` is dropped.
///
/// Config types are automatically registered if they are accessed with `Get`
/// somewhere in the program.
class Config final {
 public:
  /// @warning Must not be used explicitly. Use `MakeTaxiConfigPtr` instead!
  static Config Parse(const DocsMap& docs_map);

  Config(Config&&) noexcept = default;
  Config& operator=(Config&&) noexcept = default;

  template <typename Key>
  const VariableOfKey<Key>& operator[](Key) const;

  template <typename Key>
  static void Unregister(Key);

  template <typename Key>
  static bool IsRegistered(Key);

  /// Deprecated, use `config[key]` instead
  template <typename T>
  const T& Get() const;

  class KeyValue;

 private:
  using ConfigId = std::size_t;

  explicit Config(const DocsMap& docs_map);

  Config(std::initializer_list<KeyValue> config_variables);

  const std::any& Get(ConfigId id) const;

  static ConfigId Register(impl::Factory factory);

  static void Unregister(ConfigId id);

  static bool IsRegistered(ConfigId id);

  // For the constructors
  friend class StorageMock;

  // Automatically registers all used config types at startup and assigns them
  // sequential ids
  template <typename Key>
  static inline const ConfigId kConfigId = Register(&impl::FactoryFor<Key>);

  std::vector<std::any> user_configs_;
};

/// @cond

template <typename Key>
const VariableOfKey<Key>& Config::operator[](Key) const {
  try {
    return std::any_cast<const VariableOfKey<Key>&>(Get(kConfigId<Key>));
  } catch (const std::exception& ex) {
    impl::WrapGetError(ex, typeid(VariableOfKey<Key>));
  }
}

template <typename Key>
void Config::Unregister(Key) {
  Unregister(kConfigId<Key>);
}

template <typename Key>
bool Config::IsRegistered(Key) {
  return IsRegistered(kConfigId<Key>);
}

template <typename T>
const T& Config::Get() const {
  return (*this)[Key<impl::ParseByConstructor<T>>{}];
}

class Config::KeyValue final {
 public:
  template <typename Key>
  KeyValue(Key, VariableOfKey<Key> value)
      : id_(kConfigId<Key>), value_(std::move(value)) {}

 private:
  friend class Config;

  ConfigId id_;
  std::any value_;
};
/// @endcond

}  // namespace taxi_config
