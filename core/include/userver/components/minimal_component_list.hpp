#pragma once

/// @file userver/components/minimal_component_list.hpp
/// @brief @copybrief components::MinimalComponentList()

#include <userver/components/component_list.hpp>

namespace components {

/// @ingroup userver_components
///
/// @brief Returns a list of components to do basic logging, component
/// initialization and configuration.
///
/// The list contains:
/// * components::Logging
/// * components::Tracer
/// * components::ManagerControllerComponent
/// * components::StatisticsStorage
/// * components::TaxiConfig
ComponentList MinimalComponentList();

}  // namespace components
