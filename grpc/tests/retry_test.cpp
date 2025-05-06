#include <userver/utest/utest.hpp>

#include <userver/dynamic_config/storage_mock.hpp>
#include <userver/ugrpc/tests/service_fixtures.hpp>

#include <tests/unit_test_client.usrv.pb.hpp>
#include <tests/unit_test_client_qos.hpp>
#include <tests/unit_test_service.usrv.pb.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class UnitTestService final : public sample::ugrpc::UnitTestServiceBase {
public:
    SayHelloResult SayHello(CallContext& /*context*/, sample::ugrpc::GreetingRequest&& request) override {
        if (++request_counter_ % 4) {
            LOG_DEBUG() << request_counter_ << ": return UNAVAILABLE";
            return grpc::Status(grpc::StatusCode::UNAVAILABLE, "");
        }
        sample::ugrpc::GreetingResponse response;
        response.set_name("Hello " + request.name());
        LOG_DEBUG() << request_counter_ << ": return OK";
        return response;
    }

private:
    std::uint64_t request_counter_{};
};

using RetryTest = ugrpc::tests::ServiceFixture<UnitTestService>;

}  // namespace

UTEST_F(RetryTest, Attempts) {
    ugrpc::client::Qos qos;
    qos.attempts = 4;
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

USERVER_NAMESPACE_END
