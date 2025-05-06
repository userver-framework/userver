#pragma once

#include <userver/utest/using_namespace_userver.hpp>

#include <userver/ugrpc/client/middlewares/base.hpp>

namespace samples::grpc::auth::client {

/// [Middleware declaration]
class AuthMiddleware final : public ugrpc::client::MiddlewareBase {
public:
    // Name of a middleware-factory that creates this middleware.
    static constexpr std::string_view kName = "grpc-auth-client";

    // 'Auth' is a group for authentication. See middlewares groups for more information.
    static inline const auto kDependency =
        middlewares::MiddlewareDependencyBuilder().InGroup<middlewares::groups::Auth>();

    AuthMiddleware();

    ~AuthMiddleware() override;

    void PreStartCall(ugrpc::client::MiddlewareCallContext& context) const override;
};

// This component creates Middleware. Name of the component is 'Middleware::kName'.
// In this case we use a short-cut for defining a middleware-factory, but you can create your own factory by
// inheritance from 'ugrpc::client::MiddlewareFactoryComponentBase'
using AuthComponent = ugrpc::client::SimpleMiddlewareFactoryComponent<AuthMiddleware>;

/// [Middleware declaration]

}  // namespace samples::grpc::auth::client
