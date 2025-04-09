#pragma once
#include <userver/utest/using_namespace_userver.hpp>

#include <userver/ugrpc/server/middlewares/base.hpp>

#include <samples/greeter_service.usrv.pb.hpp>

namespace functional_tests {

class MySecondMiddleware final : public ugrpc::server::MiddlewareBase {
public:
    MySecondMiddleware() = default;

    void PostRecvMessage(ugrpc::server::MiddlewareCallContext& context, google::protobuf::Message& request)
        const override;

    void PreSendMessage(ugrpc::server::MiddlewareCallContext& context, google::protobuf::Message& response)
        const override;
};

class MySecondMiddlewareComponent final : public ugrpc::server::MiddlewareFactoryComponentBase {
public:
    static constexpr std::string_view kName = "my-second-middleware-server";

    MySecondMiddlewareComponent(const components::ComponentConfig& config, const components::ComponentContext& ctx)
        : ugrpc::server::MiddlewareFactoryComponentBase(config, ctx),
          middleware_(std::make_shared<MySecondMiddleware>()) {}

    std::shared_ptr<const ugrpc::server::MiddlewareBase> CreateMiddleware(
        const ugrpc::server::ServiceInfo&,
        const yaml_config::YamlConfig& middleware_config
    ) const override;

private:
    std::shared_ptr<ugrpc::server::MiddlewareBase> middleware_;
};

}  // namespace functional_tests
