#pragma once

/// @file userver/components/common_component_list.hpp
/// @brief @copybrief components::CommonComponentList()

#include <userver/components/component_list.hpp>

USERVER_NAMESPACE_BEGIN

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
/// * components::DynamicConfigClient
/// * components::DynamicConfigClientUpdater
/// * components::Logging
/// * components::Tracer
/// * components::ManagerControllerComponent
/// * components::StatisticsStorage
/// * components::SystemStatisticsCollector
/// * components::DynamicConfig
ComponentList CommonComponentList();

}  // namespace components

USERVER_NAMESPACE_END
