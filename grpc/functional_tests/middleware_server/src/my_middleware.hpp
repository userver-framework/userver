#pragma once
#include <userver/utest/using_namespace_userver.hpp>

#include <userver/ugrpc/server/middlewares/base.hpp>

#include <samples/greeter_service.usrv.pb.hpp>

namespace functional_tests {

class MyMiddleware final : public ugrpc::server::MiddlewareBase {
public:
    static constexpr std::string_view kName = "my-middleware-server";

    static inline const auto kDependency = middlewares::MiddlewareDependencyBuilder();

    explicit MyMiddleware() = default;

    void CallRequestHook(const ugrpc::server::MiddlewareCallContext& context, google::protobuf::Message& request)
        override;

    void CallResponseHook(const ugrpc::server::MiddlewareCallContext& context, google::protobuf::Message& response)
        override;

    void Handle(ugrpc::server::MiddlewareCallContext& context) const override;
};

using MyMiddlewareComponent = ugrpc::server::SimpleMiddlewareFactoryComponent<MyMiddleware>;

}  // namespace functional_tests
