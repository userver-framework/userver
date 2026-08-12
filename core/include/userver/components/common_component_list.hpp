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
/// * everything from @ref components::MinimalComponentList
/// * @ref components::LoggingConfigurator
/// * @ref components::DumpConfigurator
/// * @ref components::TestsuiteSupport
/// * @ref components::SystemStatisticsCollector
/// * @ref components::HttpClientCore (default and "http-client-statistics-core")
/// * @ref components::HttpClient (default and "http-client-statistics")
/// * @ref clients::dns::Component
/// * @ref components::DynamicConfigClient
/// * @ref components::DynamicConfigClientUpdater
/// * @ref engine::TaskProcessorsLoadMonitor
ComponentList CommonComponentList();

}  // namespace components

USERVER_NAMESPACE_END
