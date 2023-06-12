#pragma once

/// @file userver/storages/secdist/component.hpp
/// @brief @copybrief components::Secdist

#include <string>

#include <userver/components/loggable_component_base.hpp>
#include <userver/storages/secdist/secdist.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {
// clang-format off

/// @ingroup userver_components
///
/// @brief Component that stores security related data (keys, passwords, ...).
///
/// The component must be configured in service config.
///
/// ## Static options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// config | path to the config file with data | ''
/// format | config format, either `json` or `yaml` | 'json'
/// missing-ok | do not terminate components load if no file found by the config option | false
/// environment-secrets-key | name of environment variable from which to load additional data | -
/// update-period | period between data updates in utils::StringToDuration() suitable format ('0s' for no updates) | 0s
/// blocking-task-processor | name of task processor for background blocking operations | --

// clang-format on

class Secdist final : public LoggableComponentBase {
 public:
  /// @ingroup userver_component_names
  /// @brief The default name of components::Secdist
  static constexpr std::string_view kName = "secdist";

  Secdist(const ComponentConfig&, const ComponentContext&);

  const storages::secdist::SecdistConfig& Get() const;

  rcu::ReadablePtr<storages::secdist::SecdistConfig> GetSnapshot() const;

  storages::secdist::Secdist& GetStorage();

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  storages::secdist::Secdist secdist_;
};

template <>
inline constexpr bool kHasValidate<Secdist> = true;

}  // namespace components

USERVER_NAMESPACE_END
