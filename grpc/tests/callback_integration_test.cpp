#include <atomic>
#include <cstddef>
#include <utility>

#include <grpcpp/server_context.h>

#include <userver/ugrpc/tests/service.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/impl/internal_tag.hpp>

#include <tests/unit_test_client.usrv.pb.hpp>
#include <tests/unit_test_service.usrv.pb.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class EchoService final : public sample::ugrpc::UnitTestServiceBase {
public:
    explicit EchoService(bool use_callback_api)
        : use_callback_api_(use_callback_api)
    {}

    SayHelloResult SayHello(CallContext& context, sample::ugrpc::GreetingRequest&& request) override {
        EXPECT_EQ(
            dynamic_cast<grpc::CallbackServerContext*>(&context.GetServerContext()) != nullptr,
            use_callback_api_
        );
        ++calls_;
        sample::ugrpc::GreetingResponse response;
        response.set_name(request.name());
        return response;
    }

    std::size_t GetCallCount() const { return calls_.load(); }

private:
    std::atomic<std::size_t> calls_{0};
    const bool use_callback_api_;
};

}  // namespace

UTEST(GrpcCallbackIntegration, RegisteredServicesUseCallbackApiWithoutServerQueues) {
    ugrpc::tests::Service<EchoService> service({.server_config = {.use_callback_api = true}}, std::in_place, true);
    EXPECT_EQ(service.GetServer().GetCompletionQueues(utils::impl::InternalTag{}), nullptr);
    auto client = service.MakeClient<sample::ugrpc::UnitTestServiceClient>();
    sample::ugrpc::GreetingRequest request;
    request.set_name("callback");
    EXPECT_EQ(client.SayHello(request).name(), request.name());
    EXPECT_EQ(service.GetService().GetCallCount(), 1);
}

USERVER_NAMESPACE_END
