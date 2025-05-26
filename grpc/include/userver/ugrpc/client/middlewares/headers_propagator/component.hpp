#pragma once

/// @file userver/ugrpc/client/middlewares/headers_propagator/component.hpp
/// @brief @copybrief ugrpc::client::middlewares::headers_propagator::Component

#include <userver/ugrpc/client/middlewares/headers_propagator/middleware.hpp>

USERVER_NAMESPACE_BEGIN

/// Server headers_propagator middleware
/// @see @ref scripts/docs/en/userver/grpc/client_middlewares.md
/// @see @ref ugrpc::client::middlewares::headers_propagator::Component
namespace ugrpc::client::middlewares::headers_propagator {

/// @ingroup userver_components userver_base_classes
///
/// @brief The component for @ref ugrpc::client::middlewares::headers_propagator::Middleware.
///
/// @see @ref scripts/docs/en/userver/grpc/client_middlewares.md

using Component = SimpleMiddlewareFactoryComponent<Middleware>;

}  // namespace ugrpc::client::middlewares::headers_propagator

template <>
inline constexpr auto components::kConfigFileMode<ugrpc::client::middlewares::headers_propagator::Component> =
    ConfigFileMode::kNotRequired;

USERVER_NAMESPACE_END
