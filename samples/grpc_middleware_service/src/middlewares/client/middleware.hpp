#pragma once

#include <userver/utest/using_namespace_userver.hpp>

#include <userver/ugrpc/client/middlewares/base.hpp>
#include <userver/ugrpc/server/middlewares/groups.hpp>

namespace sample::grpc::auth::client {

/// [gRPC middleware sample - Middleware declaration]
class Middleware final : public ugrpc::client::MiddlewareBase {
public:
    // Name of a middleware-factory that creates this middleware.
    static constexpr std::string_view kName = "grpc-auth-client";

    // 'User' is a default group of user middlewares. See middlewares groups for more information.
    static inline const auto kDependency =
        ugrpc::middlewares::MiddlewareDependencyBuilder().InGroup<ugrpc::server::groups::User>();

    Middleware();

    ~Middleware() override;

    void PreStartCall(ugrpc::client::MiddlewareCallContext& context) const override;
};

// This component creates Middleware. Name of component is 'Middleware::kName'.
// In this case we use a short-cut for defining a middleware-factory, but you can declare your own factory by
// inheritance from 'ugrpc::client::MiddlewareFactoryComponentBase'
using Component = ugrpc::client::SimpleMiddlewareFactoryComponent<Middleware>;

/// [gRPC middleware sample - Middleware declaration]

}  // namespace sample::grpc::auth::client
