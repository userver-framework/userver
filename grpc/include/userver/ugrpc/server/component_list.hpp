#pragma once

/// @file userver/ugrpc/server/component_list.hpp
/// @brief Two common component lists for grpc-server (default and minimal)

#include <userver/components/component_list.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

/// @ingroup userver_components
///
/// @brief Returns a list of components to do a minimal grpc server configuration
///
/// The list contains:
/// * ugrpc::server::ServerComponent
/// * ugrpc::server::MiddlewarePipelineComponent
/// * ugrpc::server::middlewares::congestion_control::Component
/// * ugrpc::server::middlewares::deadline_propagation::Component
/// * ugrpc::server::middlewares::log::Component
components::ComponentList MinimalComponentList();

/// @ingroup userver_components
///
/// @brief Returns a list of components to do a default grpc server configuration
///
/// The list contains:
/// * ugrpc::server::ServerComponent
/// * ugrpc::server::MiddlewarePipelineComponent
/// * ugrpc::server::middlewares::baggage::Component
/// * ugrpc::server::middlewares::congestion_control::Component
/// * ugrpc::server::middlewares::deadline_propagation::Component
/// * ugrpc::server::middlewares::headers_propagator::Component
/// * ugrpc::server::middlewares::log::Component
components::ComponentList DefaultComponentList();

}  // namespace ugrpc::server

USERVER_NAMESPACE_END
