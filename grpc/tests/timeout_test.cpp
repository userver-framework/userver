#include <userver/utest/utest.hpp>

#include <userver/engine/sleep.hpp>

#include <userver/ugrpc/tests/service_fixtures.hpp>

#include <tests/deadline_helpers.hpp>
#include <tests/unit_test_client.usrv.pb.hpp>
#include <tests/unit_test_client_qos.hpp>
#include <tests/unit_test_service.usrv.pb.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class UnitTestService final : public sample::ugrpc::UnitTestServiceBase {
public:
    SayHelloResult SayHello(CallContext& context, sample::ugrpc::GreetingRequest&& request) override {
        LOG_DEBUG() << request_counter_ << " request attempt: now=" << std::chrono::system_clock::now()
                    << ", deadline=" << context.GetServerContext().deadline();
        if (++request_counter_ % 4) {
            engine::InterruptibleSleepFor(tests::kLongTimeout + tests::kShortTimeout);
            EXPECT_TRUE(context.GetServerContext().IsCancelled());
            // this status should not reach client because of 'perAttemptRecvTimeout'
            LOG_DEBUG() << request_counter_ << ": return ABORTED";
            return grpc::Status(grpc::StatusCode::ABORTED, "");
        }
        sample::ugrpc::GreetingResponse response;
        response.set_name("Hello " + request.name());
        LOG_DEBUG() << request_counter_ << ": return OK";
        return response;
    }

private:
    std::uint64_t request_counter_{};
};

using TimeoutTest = ugrpc::tests::ServiceFixture<UnitTestService>;

}  // namespace

UTEST_F(TimeoutTest, QosTimeout) {
    ugrpc::client::Qos qos;
    qos.attempts = 4;
    qos.timeout = tests::kLongTimeout;
    ugrpc::client::ClientQos client_qos;
    client_qos.methods.SetDefault(qos);
    const auto config = std::vector<dynamic_config::KeyValue>{{tests::kUnitTestClientQos, client_qos}};
    ExtendDynamicConfig(config);

    ugrpc::client::ClientSettings client_settings;
    client_settings.client_name = "test";
    client_settings.endpoint = GetEndpoint();
    client_settings.client_qos = &tests::kUnitTestClientQos;
    auto client = GetClientFactory().MakeClient<sample::ugrpc::UnitTestServiceClient>(std::move(client_settings));

    sample::ugrpc::GreetingRequest request;
    request.set_name("testname");

    sample::ugrpc::GreetingResponse response;
    UEXPECT_NO_THROW(response = client.SayHello(request));
}

UTEST_F(TimeoutTest, CallOptionsTimeout) {
    auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();

    sample::ugrpc::GreetingRequest request;
    request.set_name("testname");

    ugrpc::client::CallOptions call_options;
    call_options.SetAttempts(4);
    call_options.SetTimeout(tests::kLongTimeout);

    sample::ugrpc::GreetingResponse response;
    UEXPECT_NO_THROW(response = client.SayHello(request, std::move(call_options)));
}

USERVER_NAMESPACE_END
