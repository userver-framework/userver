#pragma once

#include <userver/utest/using_namespace_userver.hpp>

#include <userver/ugrpc/server/middlewares/base.hpp>

namespace samples::grpc::auth::server {

/// [Middleware declaration]
class Middleware final : public ugrpc::server::MiddlewareBase {
public:
    // Name of a middleware-factory that creates this middleware.
    static constexpr std::string_view kName = "grpc-server-auth";

    // 'Auth' is a group for auth authentication. See middlewares groups for more information.
    static inline const auto kDependency =
        middlewares::MiddlewareDependencyBuilder().InGroup<middlewares::groups::Auth>();

    Middleware();

    void OnCallStart(ugrpc::server::MiddlewareCallContext& context) const override;
};

// This component creates Middleware. Name of the component is 'Middleware::kName'.
// In this case we use a short-cut for defining a middleware-factory, but you can create your own factory by
// inheritance from 'ugrpc::server::MiddlewareFactoryComponentBase'
using AuthComponent = ugrpc::server::SimpleMiddlewareFactoryComponent<Middleware>;
/// [Middleware declaration]

}  // namespace samples::grpc::auth::server
