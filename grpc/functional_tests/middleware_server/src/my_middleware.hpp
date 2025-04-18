#pragma once
#include <userver/utest/using_namespace_userver.hpp>

#include <userver/ugrpc/server/middlewares/base.hpp>

#include <samples/greeter_service.usrv.pb.hpp>

namespace functional_tests {

/// [gRPC CallRequestHook declaration example]
class MyMiddleware final : public ugrpc::server::MiddlewareBase {
public:
    static constexpr std::string_view kName = "my-middleware-server";

    static inline const auto kDependency = middlewares::MiddlewareDependencyBuilder();

    MyMiddleware() = default;

    void PostRecvMessage(ugrpc::server::MiddlewareCallContext& context, google::protobuf::Message& request)
        const override;

    void PreSendMessage(ugrpc::server::MiddlewareCallContext& context, google::protobuf::Message& response)
        const override;
};

// There isn't a special logic to construct that middleware (doesn't have static config options) => use short-cut
using MyMiddlewareComponent = ugrpc::server::SimpleMiddlewareFactoryComponent<MyMiddleware>;
/// [gRPC CallRequestHook declaration example]

}  // namespace functional_tests
