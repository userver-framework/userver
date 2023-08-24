#pragma once

/// @file userver/dynamic_config/fallbacks/component.hpp
/// @brief @copybrief components::DynamicConfigFallbacks

#include <string_view>

#include <userver/components/component_fwd.hpp>
#include <userver/components/loggable_component_base.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/dynamic_config/updates_sink/component.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

// clang-format off

/// @ingroup userver_components
///
/// @brief Component that setup runtime configs based on fallbacks from file.
///
/// ## Static options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// updates-sink | name of the component derived from components::DynamicConfigUpdatesSinkBase to be used for storing fallback config | dynamic-config
/// fallback-path | a path to the fallback config | -
/// overrides-path | a path to the file containing overrides for a subset of fallback config parameters | -
///
/// If you use this component, you have to disable loading of other updaters
/// (like DynamicConfigClientUpdater) as there must be only a single component
/// that sets config values.
///
/// ## Static configuration example:
///
/// @snippet core/src/components/component_list_test.hpp  Sample dynamic config fallback component

// clang-format on
class DynamicConfigFallbacks final : public LoggableComponentBase {
 public:
  /// @ingroup userver_component_names
  /// @brief The default name of components::DynamicConfigFallbacks
  static constexpr std::string_view kName = "dynamic-config-fallbacks";

  DynamicConfigFallbacks(const ComponentConfig&, const ComponentContext&);

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  DynamicConfigUpdatesSinkBase& updates_sink_;
};

template <>
inline constexpr bool kHasValidate<DynamicConfigFallbacks> = true;

}  // namespace components

USERVER_NAMESPACE_END
