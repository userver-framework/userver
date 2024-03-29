#pragma once

/// @file userver/components/minimal_server_component_list.hpp
/// @brief @copybrief components::MinimalServerComponentList()

#include <userver/components/component_list.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

/// @ingroup userver_components
///
/// @brief Returns a list of components to start a basic HTTP server.
///
/// The list contains:
/// * all of components::MinimalComponentList
/// * components::Server
/// * components::AuthCheckerSettings
ComponentList MinimalServerComponentList();

}  // namespace components

USERVER_NAMESPACE_END
