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
/// * everything from components::MinimalComponentList
/// * components::LoggingConfigurator
/// * components::DumpConfigurator
/// * components::TestsuiteSupport
/// * components::SystemStatisticsCollector
/// * components::HttpClient (default and "http-client-statistics")
/// * clients::dns::Component
/// * components::DynamicConfigClient
/// * components::DynamicConfigClientUpdater
/// * engine::TaskProcessorsLoadMonitor
ComponentList CommonComponentList();

}  // namespace components

USERVER_NAMESPACE_END
