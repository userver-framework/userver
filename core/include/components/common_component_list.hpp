#pragma once

/// @file components/common_component_list.hpp
/// @brief @copybrief components::CommonComponentList()

#include <components/component_list.hpp>

namespace components {

/// @ingroup userver_components
///
/// The most common list of components with runtime config updates and
/// HTTP client. The list could be used in components::Run() and
/// components::RunOnce().
///
/// The list contains:
/// * components::LoggingConfigurator
/// * components::TestsuiteSupport
/// * congestion_control::Component
/// * components::HttpClient (default and "http-client-statistics")
/// * components::TaxiConfigClient
/// * components::TaxiConfigClientUpdater
/// * components::Logging
/// * components::Tracer
/// * components::ManagerControllerComponent
/// * components::StatisticsStorage
/// * components::TaxiConfig
ComponentList CommonComponentList();

}  // namespace components
