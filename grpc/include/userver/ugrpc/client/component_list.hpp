#pragma once

/// @file userver/ugrpc/client/component_list.hpp
/// @brief Two common component lists for grpc-client (default and minimal)

#include <userver/components/component_list.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

/// @ingroup userver_components
///
/// @brief Returns a list of components to do a minimal grpc client configuration.
///
/// The list contains:
/// * ugrpc::client::CommonComponent
/// * ugrpc::client::MiddlewarePipelineComponent
/// * ugrpc::client::middlewares::deadline_propagation::Component
/// * ugrpc::client::middlewares::log::Component
components::ComponentList MinimalComponentList();

/// @ingroup userver_components
///
/// @brief Returns a list of components to do a default grpc client configuration.
///
/// The list contains:
/// * ugrpc::client::CommonComponent
/// * ugrpc::client::MiddlewarePipelineComponent
/// * ugrpc::client::middlewares::baggage::Component
/// * ugrpc::client::middlewares::deadline_propagation::Component
/// * ugrpc::client::middlewares::log::Component
/// * ugrpc::client::middlewares::headers_propagator::Component
components::ComponentList DefaultComponentList();

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
