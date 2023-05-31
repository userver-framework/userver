#pragma once

/// @file userver/storages/secdist/component.hpp
/// @brief @copybrief components::DefaultSecdistProvider

#include <string>

#include <userver/components/loggable_component_base.hpp>
#include <userver/storages/secdist/provider.hpp>
#include <userver/storages/secdist/secdist.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::secdist {

class DefaultLoader final : public storages::secdist::SecdistProvider {
 public:
  struct Settings {
    std::string config_path;
    SecdistFormat format{SecdistFormat::kJson};
    bool missing_ok{false};
    std::optional<std::string> environment_secrets_key;
    engine::TaskProcessor* blocking_task_processor{nullptr};
  };

  explicit DefaultLoader(Settings settings);

  formats::json::Value Get() const override;

 private:
  Settings settings_;
};

}  // namespace storages::secdist

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
/// blocking-task-processor | name of task processor for background blocking operations | --

// clang-format on

class DefaultSecdistProvider final : public LoggableComponentBase,
                                     public storages::secdist::SecdistProvider {
 public:
  /// @ingroup userver_component_names
  /// @brief The default name of components::DefaultSecdistProvider
  static constexpr std::string_view kName = "default-secdist-provider";

  DefaultSecdistProvider(const ComponentConfig&, const ComponentContext&);

  formats::json::Value Get() const override;

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  storages::secdist::DefaultLoader loader_;
};

template <>
inline constexpr bool kHasValidate<DefaultSecdistProvider> = true;

}  // namespace components

USERVER_NAMESPACE_END
