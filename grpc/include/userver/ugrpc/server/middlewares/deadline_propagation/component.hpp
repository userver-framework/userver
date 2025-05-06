#pragma once

/// @file userver/ugrpc/server/middlewares/deadline_propagation/component.hpp
/// @brief @copybrief ugrpc::server::middlewares::deadline_propagation::Component

#include <userver/ugrpc/server/middlewares/deadline_propagation/middleware.hpp>

USERVER_NAMESPACE_BEGIN

/// Server deadline propagation middleware
/// @see @ref scripts/docs/en/userver/grpc/server_middlewares.md
/// @see @ref ugrpc::server::middlewares::deadline_propagation::Component
namespace ugrpc::server::middlewares::deadline_propagation {

// clang-format off

/// @ingroup userver_base_classes
///
/// @brief Component for gRPC server deadline propagation
/// @see @ref scripts/docs/en/userver/deadline_propagation.md
///
/// The component does **not** have any options for service config.
///
/// ## Static configuration example:
///
/// @snippet grpc/functional_tests/basic_chaos/static_config.yaml Sample grpc server deadline propagation middleware component config
///
/// @see @ref scripts/docs/en/userver/grpc/server_middlewares.md

// clang-format on

using Component = SimpleMiddlewareFactoryComponent<Middleware>;

}  // namespace ugrpc::server::middlewares::deadline_propagation

template <>
inline constexpr auto components::kConfigFileMode<ugrpc::server::middlewares::deadline_propagation::Component> =
    ConfigFileMode::kNotRequired;

USERVER_NAMESPACE_END
