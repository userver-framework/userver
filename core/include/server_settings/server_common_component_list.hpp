#pragma once

/// @file server_settings/server_common_component_list.hpp
/// @brief @copybrief server_settings::ServerCommonComponentList()

#include <components/component_list.hpp>
#include <server_settings/http_server_settings_component.hpp>

namespace server_settings {
namespace impl {

components::ComponentList ServerCommonComponentList();

}  // namespace impl

/// @ingroup userver_components
///
/// The most common list of components to start a fully functional server.
///
/// The list contains:
/// * components::Server
/// * server::handlers::TestsControl
/// * server::handlers::ServerMonitor
/// * server::handlers::LogLevel
/// * server::handlers::InspectRequests
/// * server::handlers::ImplicitOptionsHttpHandler
/// * server::handlers::Jemalloc
/// * components::AuthCheckerSettings
/// * congestion_control::Component
/// * components::HttpServerSettings from HttpServerSettingsConfigNames
template <typename HttpServerSettingsConfigNames = ConfigNamesDefault>
components::ComponentList ServerCommonComponentList() {
  return impl::ServerCommonComponentList()
      .Append<components::HttpServerSettings<HttpServerSettingsConfigNames>>();
}

}  // namespace server_settings
