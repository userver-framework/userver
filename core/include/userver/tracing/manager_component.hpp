#pragma once

/// @file userver/tracing/manager_component.hpp
/// @brief TracingManager base and default components

#include <string>

#include <userver/components/loggable_component_base.hpp>
#include <userver/tracing/manager.hpp>
#include <userver/tracing/span.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing {

/// @brief Base component for implementing TracingManager component
// NOLINTNEXTLINE(fuchsia-multiple-inheritance)
class TracingManagerComponentBase : public components::LoggableComponentBase,
                                    public TracingManagerBase {
 public:
  TracingManagerComponentBase(const components::ComponentConfig&,
                              const components::ComponentContext&);
};

// clang-format off

/// @ingroup userver_components
///
/// @brief Locator component that provides access to the actual TracingManager
/// that will be used in handlers and clients unless specified otherwise
///
/// The component can be configured in service config.
/// If the config is not provided, then tracing::kDefaultTracingManager will be used
///
/// ## Static options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// component-name | name of the component, that implements TracingManagerComponentBase | <use kDefaultTracingManager>
///
// clang-format on
class DefaultTracingManagerLocator final
    : public components::LoggableComponentBase {
 public:
  static constexpr std::string_view kName = "tracing-manager-locator";

  DefaultTracingManagerLocator(const components::ComponentConfig&,
                               const components::ComponentContext&);

  const TracingManagerBase& GetTracingManager() const;

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  const TracingManagerBase& tracing_manager_;
};

}  // namespace tracing

template <>
inline constexpr bool
    components::kHasValidate<tracing::DefaultTracingManagerLocator> = true;

template <>
inline constexpr auto
    components::kConfigFileMode<tracing::DefaultTracingManagerLocator> =
        ConfigFileMode::kNotRequired;

USERVER_NAMESPACE_END
