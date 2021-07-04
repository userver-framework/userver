#pragma once

/// @file userver/components/common_component_list.hpp
/// @brief @copybrief components::CommonComponentList()

#include <userver/components/component_list.hpp>

namespace components {

/// @ingroup userver_components
///
/// @brief Returns the most common list of components with runtime config
/// updates and HTTP client.
///
/// The list contains:
/// * components::LoggingConfigurator
/// * components::TestsuiteSupport
/// * components::HttpClient (default and "http-client-statistics")
/// * components::TaxiConfigClient
/// * components::TaxiConfigClientUpdater
/// * components::Logging
/// * components::Tracer
/// * components::ManagerControllerComponent
/// * components::StatisticsStorage
/// * components::SystemStatisticsCollector
/// * components::TaxiConfig
ComponentList CommonComponentList();

}  // namespace components
