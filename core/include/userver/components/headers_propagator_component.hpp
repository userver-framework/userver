#pragma once

/// @file userver/components/headers_propagator_component.hpp
/// @brief HeadersPropagatorComponent and default components

#include <memory>

#include <userver/components/loggable_component_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http {
class HeadersPropagator;
}  // namespace server::http

namespace components {

// clang-format on

/// @ingroup userver_components
///
/// @brief Headers Propagator Component can scrape configured headers
/// and then enrich HttpClient request with them.
///
/// The component can be configured in service config.
///
/// ## Static options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// headers | List of headers to propagate | []
///
// clang-format on
class HeadersPropagatorComponent final
    : public components::LoggableComponentBase {
 public:
  /// @ingroup userver_component_names
  /// @brief The default name of components::HeadersPropagatorComponent
  /// component
  static constexpr std::string_view kName = "headers-propagator";

  HeadersPropagatorComponent(const components::ComponentConfig&,
                             const components::ComponentContext&);
  ~HeadersPropagatorComponent() override;

  static yaml_config::Schema GetStaticConfigSchema();

  server::http::HeadersPropagator& Get();

 private:
  std::unique_ptr<server::http::HeadersPropagator> propagator_;
};

}  // namespace components

template <>
inline constexpr bool
    components::kHasValidate<components::HeadersPropagatorComponent> = true;

template <>
inline constexpr auto
    components::kConfigFileMode<components::HeadersPropagatorComponent> =
        ConfigFileMode::kNotRequired;

USERVER_NAMESPACE_END
