#pragma once

/// @file userver/dynamic_config/snapshot.hpp
/// @brief @copybrief dynamic_config::Snapshot

#include <cstdint>
#include <string>
#include <type_traits>

#include <userver/dynamic_config/impl/snapshot.hpp>
#include <userver/dynamic_config/impl/to_json.hpp>
#include <userver/formats/json_fwd.hpp>
#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace dynamic_config {

/// A strong typedef for usage in dynamic_config::Key constructors.
struct DefaultAsJsonString final {
  constexpr explicit DefaultAsJsonString(std::string_view json_string);

  std::string_view json_string;
};

/// A config name-value pair for usage in dynamic_config::Key constructors.
struct ConfigDefault final {
  template <typename T>
  ConfigDefault(std::string_view name, const T& value);

  ConfigDefault(std::string_view name, DefaultAsJsonString default_json);

  std::string_view name;
  std::string default_json;
};

/// A tag type for usage in dynamic_config::Key constructors.
struct ConstantConfig final {
  constexpr explicit ConstantConfig() = default;
};

/// @brief A config key is a unique identifier for a config variable
/// @snippet core/src/components/logging_configurator.cpp  key
template <typename Variable>
class Key final {
 public:
  /// The type of the parsed config variable.
  using VariableType = Variable;

  using JsonParser = Variable (*)(const formats::json::Value&);
  using DocsMapParser = Variable (*)(const DocsMap&);

  /// @brief The constructor for a trivial `VariableType`, e.g. `bool`, integer,
  /// `double`, `string`. The default is passed by value.
  ///
  /// Usage example:
  /// @snippet server/handlers/http_server_settings.cpp  bool config sample
  Key(std::string_view name, const VariableType& default_value);

  /// @brief The constructor for a non-trivial `VariableType`. The default is
  /// passed as a JSON string.
  ///
  /// Uses formats::json::Value `Parse` customization point function to parse
  /// `VariableType`.
  ///
  /// Usage example:
  /// @snippet server/handlers/http_server_settings.cpp
  Key(std::string_view name, DefaultAsJsonString default_json);

  /// @brief The constructor that provides a special parser from JSON.
  /// @warning Prefer the constructors above whenever possible.
  /// @details Can be used when generic `Parse` is not applicable. Sometimes
  /// used to add validation, e.g. minimum, maximum, string pattern, etc.
  Key(std::string_view name, JsonParser parser,
      DefaultAsJsonString default_json);

  /// @brief The constructor that parses multiple JSON config items
  /// into a single C++ object.
  /// @warning Prefer to use a separate `Key` per JSON config item and use the
  /// constructors above whenever possible.
  ///
  /// Usage example:
  /// @snippet clients/http/component.cpp  docs map config sample
  template <std::size_t N>
  Key(DocsMapParser parser, const ConfigDefault (&default_json_map)[N]);

  /// Creates a config that always has the same value.
  Key(ConstantConfig, VariableType value);

  /// @cond
  Key(impl::InternalTag, std::string_view name);

  Key(impl::InternalTag, DocsMapParser parser);
  /// @endcond

  Key(const Key&) noexcept = delete;
  Key& operator=(const Key&) noexcept = delete;

  /// @returns the name of the config variable, as passed at the construction.
  std::string_view GetName() const noexcept;

  /// Parses the config. Useful only in some very niche scenarios. The config
  /// value should be typically be retrieved from dynamic_config::Snapshot,
  /// which is obtained from components::DynamicConfig in production or from
  /// dynamic_config::StorageMock in unit tests.
  VariableType Parse(const DocsMap& docs_map) const;

 private:
  friend struct impl::ConfigIdGetter;

  const impl::ConfigId id_;
};

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
class Snapshot final {
 public:
  Snapshot(const Snapshot&);
  Snapshot& operator=(const Snapshot&);

  Snapshot(Snapshot&&) noexcept;
  Snapshot& operator=(Snapshot&&) noexcept;

  ~Snapshot();

  /// Used to access individual configs in the type-safe config map
  template <typename VariableType>
  const VariableType& operator[](const Key<VariableType>& key) const&;

  /// Used to access individual configs in the type-safe config map
  template <typename VariableType>
  const VariableType& operator[](const Key<VariableType>&) &&;

  /// @cond
  // No longer supported, use `config[key]` instead
  template <typename T>
  const T& Get() const&;

  // No longer supported, use `config[key]` instead
  template <typename T>
  const T& Get() &&;
  /// @endcond

 private:
  // for the constructor
  friend class Source;
  friend class impl::StorageData;

  explicit Snapshot(const impl::StorageData& storage);

  const impl::SnapshotData& GetData() const;

  struct Impl;
  utils::FastPimpl<Impl, 16, 8> impl_;
};

// ========================== Implementation follows ==========================

constexpr DefaultAsJsonString::DefaultAsJsonString(std::string_view json_string)
    : json_string(json_string) {}

template <typename T>
ConfigDefault::ConfigDefault(std::string_view name, const T& value)
    : name(name), default_json(impl::ToJsonString(value)) {}

template <typename Variable>
Key<Variable>::Key(std::string_view name, const VariableType& default_value)
    : id_(impl::Register(
          std::string{name},
          [name = std::string{name}](const auto& docs_map) -> std::any {
            return impl::DocsMapGet(docs_map, name).template As<VariableType>();
          },
          impl::ValueToDocsMapString(name, default_value))) {}

template <typename Variable>
Key<Variable>::Key(std::string_view name, DefaultAsJsonString default_json)
    : id_(impl::Register(
          std::string{name},
          [name = std::string{name}](const auto& docs_map) -> std::any {
            return impl::DocsMapGet(docs_map, name).template As<VariableType>();
          },
          impl::SingleToDocsMapString(name, default_json.json_string))) {}

template <typename Variable>
Key<Variable>::Key(std::string_view name, JsonParser parser,
                   DefaultAsJsonString default_json)
    : id_(impl::Register(
          std::string{name},
          [name = std::string{name}, parser](const auto& docs_map) -> std::any {
            return parser(impl::DocsMapGet(docs_map, name));
          },
          impl::SingleToDocsMapString(name, default_json.json_string))) {}

template <typename Variable>
template <std::size_t N>
Key<Variable>::Key(DocsMapParser parser,
                   const ConfigDefault (&default_json_map)[N])
    : id_(impl::Register(
          std::string{},
          [parser](const DocsMap& docs_map) -> std::any {
            return parser(docs_map);
          },
          impl::MultipleToDocsMapString(default_json_map, N))) {}

template <typename Variable>
Key<Variable>::Key(ConstantConfig /*tag*/, VariableType value)
    : id_(impl::Register(
          std::string{},
          [value = std::move(value)](const DocsMap& /*unused*/) {
            return value;
          },
          "{}")) {}

template <typename Variable>
Key<Variable>::Key(impl::InternalTag, std::string_view name)
    : id_(impl::Register(
          std::string{name},
          [name = std::string{name}](const auto& docs_map) -> std::any {
            return impl::DocsMapGet(docs_map, name).template As<VariableType>();
          },
          "{}")) {}

template <typename Variable>
Key<Variable>::Key(impl::InternalTag, DocsMapParser parser)
    : id_(impl::Register(
          std::string{},
          [parser](const DocsMap& docs_map) -> std::any {
            return parser(docs_map);
          },
          "{}")) {}

template <typename VariableType>
std::string_view Key<VariableType>::GetName() const noexcept {
  return impl::GetName(id_);
}

template <typename VariableType>
VariableType Key<VariableType>::Parse(const DocsMap& docs_map) const {
  return std::any_cast<VariableType>(impl::MakeConfig(id_, docs_map));
}

template <typename VariableType>
const VariableType& Snapshot::operator[](const Key<VariableType>& key) const& {
  return GetData().Get<VariableType>(impl::ConfigIdGetter::Get(key));
}

template <typename VariableType>
const VariableType& Snapshot::operator[](const Key<VariableType>&) && {
  static_assert(!sizeof(VariableType),
                "keep the Snapshot before using, please");
}

template <typename T>
const T& Snapshot::Get() const& {
  return (*this)[T::kDeprecatedKey];
}

template <typename T>
const T& Snapshot::Get() && {
  static_assert(!sizeof(T), "keep the Snapshot before using, please");
}

}  // namespace dynamic_config

USERVER_NAMESPACE_END
