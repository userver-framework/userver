#pragma once

/// @file components/minimal_server_component_list.hpp
/// @brief @copybrief components::MinimalServerComponentList()

#include <components/component_list.hpp>

namespace components {

/// @ingroup userver_components
///
/// @brief Returns a list of components to start a basic HTTP server.
///
/// The list contains:
/// * components::HttpServerSettings
/// * components::Server
/// * components::AuthCheckerSettings
/// * components::Logging
/// * components::Tracer
/// * components::ManagerControllerComponent
/// * components::StatisticsStorage
/// * components::TaxiConfig
ComponentList MinimalServerComponentList();

}  // namespace components
