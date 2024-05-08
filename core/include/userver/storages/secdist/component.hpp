#pragma once

/// @file userver/storages/secdist/component.hpp
/// @brief @copybrief components::Secdist

#include <string>

#include <userver/storages/secdist/component_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {
// clang-format off

/// @ingroup userver_components
///
/// @brief Default implementation of components::SecdistComponentBase.
///
/// The component must be configured in service config.
///
/// ## Static configuration example:
///
/// @snippet samples/redis_service/static_config.yaml Sample secdist static config
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

class Secdist final : public SecdistComponentBase {
 public:
  /// @ingroup userver_component_names
  /// @brief The default name of components::Secdist
  static constexpr std::string_view kName = "secdist";

  Secdist(const ComponentConfig&, const ComponentContext&);

  static yaml_config::Schema GetStaticConfigSchema();
};
/// [Sample secdist - default secdist]
template <>
inline constexpr bool kHasValidate<Secdist> = true;

}  // namespace components

USERVER_NAMESPACE_END
