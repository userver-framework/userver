#pragma once
#include <userver/utest/using_namespace_userver.hpp>

#include <userver/ugrpc/server/middlewares/base.hpp>

#include <samples/greeter_service.usrv.pb.hpp>

namespace functional_tests {

class MySecondMiddleware final : public ugrpc::server::MiddlewareBase {
 public:
  explicit MySecondMiddleware() = default;

  void CallRequestHook(const ugrpc::server::MiddlewareCallContext& context,
                       google::protobuf::Message& request) override;

  void CallResponseHook(const ugrpc::server::MiddlewareCallContext& context,
                        google::protobuf::Message& response) override;

  void Handle(ugrpc::server::MiddlewareCallContext& context) const override;
};

class MySecondMiddlewareComponent final
    : public ugrpc::server::MiddlewareComponentBase {
 public:
  static constexpr std::string_view kName = "my-second-middleware-server";

  MySecondMiddlewareComponent(const components::ComponentConfig& config,
                              const components::ComponentContext& ctx)
      : ugrpc::server::MiddlewareComponentBase(config, ctx),
        middleware_(std::make_shared<MySecondMiddleware>()) {}

  std::shared_ptr<ugrpc::server::MiddlewareBase> GetMiddleware() override;

 private:
  std::shared_ptr<ugrpc::server::MiddlewareBase> middleware_;
};

}  // namespace functional_tests
