#pragma once

/// @file userver/components/common_server_component_list.hpp
/// @brief @copybrief components::CommonServerComponentList()

#include <userver/components/component_list.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

/// @ingroup userver_components
///
/// @brief Returns the most common list of components to start a fully
/// functional server.
///
/// The list contains:
/// * components::Server
/// * server::handlers::DnsClientControl
/// * server::handlers::DynamicDebugLog
/// * server::handlers::ImplicitOptionsHttpHandler
/// * server::handlers::InspectRequests
/// * server::handlers::Jemalloc
/// * server::handlers::LogLevel
/// * server::handlers::OnLogRotate
/// * server::handlers::ServerMonitor
/// * server::handlers::TestsControl
/// * components::AuthCheckerSettings
/// * congestion_control::Component
/// * components::HttpServerSettings
/// * tracing::DefaultTracingManagerLocator
ComponentList CommonServerComponentList();

}  // namespace components

USERVER_NAMESPACE_END
