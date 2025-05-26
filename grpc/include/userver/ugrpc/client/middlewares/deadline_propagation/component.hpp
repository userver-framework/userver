#pragma once

/// @file userver/ugrpc/client/middlewares/deadline_propagation/component.hpp
/// @brief @copybrief ugrpc::client::middlewares::deadline_propagation::Component

#include <userver/ugrpc/client/middlewares/deadline_propagation/middleware.hpp>

USERVER_NAMESPACE_BEGIN

/// Client logging middleware
/// @see @ref scripts/docs/en/userver/grpc/client_middlewares.md
/// @see @ref ugrpc::client::middlewares::deadline_propagation::Component
namespace ugrpc::client::middlewares::deadline_propagation {

// clang-format off

/// @ingroup userver_components
///
/// @brief Component for gRPC client deadline_propagation. Update deadline
/// from TaskInheritedData if it exists and more strict than
/// context deadline.
/// @see @ref scripts/docs/en/userver/deadline_propagation.md
///
/// The component does **not** have any options for service config.
///
/// ## Static configuration example:
///
/// @snippet grpc/functional_tests/basic_chaos/static_config.yaml Sample grpc client deadline propagation middleware component config
///
/// @see @ref scripts/docs/en/userver/grpc/client_middlewares.md

// clang-format on

using Component = SimpleMiddlewareFactoryComponent<Middleware>;

}  // namespace ugrpc::client::middlewares::deadline_propagation

template <>
inline constexpr auto components::kConfigFileMode<ugrpc::client::middlewares::deadline_propagation::Component> =
    ConfigFileMode::kNotRequired;

USERVER_NAMESPACE_END
