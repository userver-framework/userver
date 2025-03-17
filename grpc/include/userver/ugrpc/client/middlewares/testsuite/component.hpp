#pragma once

/// @file userver/ugrpc/client/middlewares/testsuite/component.hpp
/// @brief @copybrief ugrpc::client::middlewares::testsuite::Component

#include <userver/ugrpc/client/middlewares/base.hpp>
#include <userver/ugrpc/client/middlewares/testsuite/middleware.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::middlewares::testsuite {

// clang-format off

/// @ingroup userver_components
///
/// @brief Component for gRPC client testsuite support
/// The component supports testsuite errors thrown from the mockserver, such as `NetworkError`, `TimeoutError`.
/// @see @ref pytest_userver.plugins.grpc.mockserver.grpc_mockserver_new "grpc_mockserver_new"
/// @see @ref pytest_userver.grpc.TimeoutError
/// @see @ref pytest_userver.grpc.NetworkError
///
/// The component does **not** have any options for service config.

// clang-format on

using Component = SimpleMiddlewareFactoryComponent<Middleware>;

}  // namespace ugrpc::client::middlewares::testsuite

template <>
inline constexpr auto components::kConfigFileMode<ugrpc::client::middlewares::testsuite::Component> =
    ConfigFileMode::kNotRequired;

USERVER_NAMESPACE_END
