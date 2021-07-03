#pragma once

/// @file components/tracer.hpp
/// @brief @copybrief components::Tracer

#include <components/component_config.hpp>
#include <components/component_context.hpp>
#include <components/impl/component_base.hpp>

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
};

}  // namespace components
