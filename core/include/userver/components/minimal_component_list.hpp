#pragma once

/// @file userver/components/minimal_component_list.hpp
/// @brief @copybrief components::MinimalComponentList()

#include <userver/components/component_list.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

/// @ingroup userver_components
///
/// @brief Returns a list of components to do basic logging, component
/// initialization and configuration.
///
/// The list contains:
/// * os_signals::ProcessorComponent
/// * components::Logging
/// * components::Tracer
/// * components::ManagerControllerComponent
/// * components::StatisticsStorage
/// * components::DynamicConfig
/// * components::DynamicConfigFallbacksComponent
/// * tracing::DefaultTracingManagerLocator
ComponentList MinimalComponentList();

}  // namespace components

USERVER_NAMESPACE_END
