#pragma once

/// @file components/minimal_component_list.hpp
/// @brief @copybrief components::MinimalComponentList()

#include <components/component_list.hpp>

namespace components {

/// Returns list of components to do basic logging, component initialization
/// and configuration. The list could be used in components::Run() and
/// components::RunOnce().
///
/// The list contains:
/// * components::Logging
/// * components::Tracer
/// * components::ManagerControllerComponent
/// * components::StatisticsStorage
/// * components::TaxiConfig
ComponentList MinimalComponentList();

}  // namespace components
