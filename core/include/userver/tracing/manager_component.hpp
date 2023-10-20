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
/// @brief Component that provides access to the actual TracingManager
/// that is used in handlers and clients.
///
/// This component allows conversion of tracing formats and allows working with
/// multiple tracing formats. For example:
/// @code
/// # yaml
/// incoming-format: ['opentelemetry', 'taxi']
/// new-requests-format: ['b3-alternative', 'opentelemetry']
/// @endcode
/// means that tracing data is extracted from OpenTelemetry headers if they
/// were received or from Yandex-Taxi specific headers. The outgoing requests
/// will have the tracing::Format::kB3Alternative headers and OpenTelemetry
/// headers at the same time.
///
/// The component can be configured in service config.
///
/// ## Static options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// component-name | name of the component, that implements TracingManagerComponentBase | <use tracing::GenericTracingManager with below settings>
/// incoming-format | Array of incoming tracing formats supported by tracing::FormatFromString | ['taxi']
/// new-requests-format | Send tracing data in those formats supported by tracing::FormatFromString | ['taxi']
///
// clang-format on
class DefaultTracingManagerLocator final
    : public components::LoggableComponentBase {
 public:
  /// @ingroup userver_component_names
  /// @brief The default name of tracing::DefaultTracingManagerLocator
  static constexpr std::string_view kName = "tracing-manager-locator";

  DefaultTracingManagerLocator(const components::ComponentConfig&,
                               const components::ComponentContext&);

  const TracingManagerBase& GetTracingManager() const;

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  GenericTracingManager default_manager_;
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
