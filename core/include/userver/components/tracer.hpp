#pragma once

/// @file userver/components/tracer.hpp
/// @brief @copybrief components::Tracer

#include <userver/components/component_fwd.hpp>
#include <userver/components/impl/component_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

// clang-format off

/// @ingroup userver_components
///
/// @brief Component that initializes the request tracing facilities.
///
/// Finds the components::Logging component and requests an optional
/// "opentracing" logger to use for Opentracing.
///
/// The component must be configured in service config.
///
/// ## Static options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// service-name | name of the service to write in traces | -
/// tracer | type of the tracer to trace, currently supported only 'native' | 'native'
///
/// ## Static configuration example:
///
/// @snippet components/common_component_list_test.cpp  Sample tracer component config

// clang-format on
class Tracer final : public impl::ComponentBase {
 public:
  static constexpr auto kName = "tracer";

  Tracer(const ComponentConfig& config, const ComponentContext& context);

  static yaml_config::Schema GetStaticConfigSchema();
};

template <>
inline constexpr bool kHasValidate<Tracer> = true;

}  // namespace components

USERVER_NAMESPACE_END
