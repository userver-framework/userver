#pragma once

/// @file userver/ugrpc/client/middlewares/baggage/component.hpp
/// @brief @copybrief ugrpc::client::middlewares::baggage::Component

#include <userver/ugrpc/client/middlewares/baggage/middleware.hpp>

USERVER_NAMESPACE_BEGIN

/// Client baggage middleware
/// @see @ref scripts/docs/en/userver/grpc/client_middlewares.md
/// @see @ref ugrpc::client::middlewares::baggage::Component
namespace ugrpc::client::middlewares::baggage {

/// @ingroup userver_components
///
/// @brief Component for gRPC client baggage
///
/// The component does **not** have any options for service config.
///
/// ## Static configuration example:
///
/// @snippet grpc/functional_tests/basic_chaos/static_config.yaml Sample grpc client baggage middleware component config
///
/// @see @ref scripts/docs/en/userver/grpc/client_middlewares.md

using Component = SimpleMiddlewareFactoryComponent<Middleware>;

}  // namespace ugrpc::client::middlewares::baggage

template <>
inline constexpr auto components::kConfigFileMode<ugrpc::client::middlewares::baggage::Component> =
    ConfigFileMode::kNotRequired;

USERVER_NAMESPACE_END
