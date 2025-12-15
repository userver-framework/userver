#include <chrono>
#include <string_view>

#include <boost/algorithm/cxx11/any_of.hpp>

#include <userver/dynamic_config/storage_mock.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/utest/log_capture_fixture.hpp>
#include <userver/utest/utest.hpp>

#include <userver/ugrpc/client/client_qos.hpp>
#include <userver/ugrpc/client/exceptions.hpp>

#include <tests/deadline_helpers.hpp>
#include <tests/timed_out_service.hpp>
#include <tests/unit_test_client.usrv.pb.hpp>
#include <tests/unit_test_client_qos.hpp>
#include <tests/unit_test_service.usrv.pb.hpp>
#include <userver/ugrpc/tests/service_fixtures.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

std::vector<dynamic_config::KeyValue> MakeQosConfig(std::chrono::milliseconds timeout) {
    ugrpc::client::Qos qos;
    qos.timeout = timeout;

    ugrpc::client::ClientQos client_qos;
    client_qos.methods.SetDefault(qos);

    return {
        {tests::kUnitTestClientQos, client_qos},
    };
}

std::vector<dynamic_config::KeyValue> MakePerRpcQosConfig(std::chrono::milliseconds timeout) {
    ugrpc::client::Qos qos;
    qos.timeout = timeout;

    ugrpc::client::ClientQos client_qos;
    client_qos.methods.Set("sample.ugrpc.UnitTestService/SayHello", qos);

    return {
        {tests::kUnitTestClientQos, client_qos},
    };
}

class UnitTestOkService final : public sample::ugrpc::UnitTestServiceBase {
public:
    SayHelloResult SayHello(CallContext& /*context*/, sample::ugrpc::GreetingRequest&& request) override {
        engine::InterruptibleSleepFor(tests::kShortTimeout);

        sample::ugrpc::GreetingResponse response;
        response.set_name("Hello " + request.name());
        return response;
    }
};

template <typename Service>
class GrpcClientQosConfigFixture : public ugrpc::tests::ServiceFixture<Service> {
public:
    ugrpc::client::ClientSettings MakeClientSettingsWithQos() {
        ugrpc::client::ClientSettings client_settings;
        client_settings.client_name = "test";
        client_settings.endpoint = this->GetEndpoint();
        client_settings.client_qos = &tests::kUnitTestClientQos;
        return client_settings;
    }
};

using GrpcClientQosConfigOk = GrpcClientQosConfigFixture<UnitTestOkService>;

class GrpcClientQosConfigExceeded : public GrpcClientQosConfigFixture<tests::TimedOutUnitTestService> {
public:
    GrpcClientQosConfigExceeded()
        : client_deadline_(engine::Deadline::FromDuration(tests::kShortTimeout)),
          long_deadline_(engine::Deadline::FromDuration(tests::kLongTimeout))
    {}

    ~GrpcClientQosConfigExceeded() override {
        EXPECT_TRUE(client_deadline_.IsReached());
        // Check that the waiting has been interrupted after kShortTimeout.
        EXPECT_FALSE(long_deadline_.IsReached());
    }

private:
    engine::Deadline client_deadline_;
    engine::Deadline long_deadline_;
};

const sample::ugrpc::GreetingRequest kRequest = [] {
    sample::ugrpc::GreetingRequest request;
    request.set_name("userver");
    return request;
}();

ugrpc::client::ClientQos MakeClientQosWithMethods(std::vector<std::string> method_paths) {
    ugrpc::client::ClientQos client_qos;
    const ugrpc::client::Qos qos = {std::nullopt, std::chrono::milliseconds{1000}};
    for (auto&& path : method_paths) {
        client_qos.methods.Set(std::move(path), qos);
    }
    client_qos.methods.SetDefault(qos);
    return client_qos;
}

constexpr std::string_view kClientQosInvalidConfigurationPath = "grpc_client_qos_invalid_configuration";

}  // namespace

UTEST_F(GrpcClientQosConfigOk, Ok) {
    auto client = GetClientFactory().MakeClient<sample::ugrpc::UnitTestServiceClient>(MakeClientSettingsWithQos());
    ExtendDynamicConfig(MakeQosConfig(utest::kMaxTestWaitTime));

    sample::ugrpc::GreetingResponse response;
    UEXPECT_NO_THROW(response = client.SayHello(kRequest));
    EXPECT_EQ(response.name(), "Hello " + kRequest.name());
}

UTEST_F(GrpcClientQosConfigOk, ContextDeadlineOverrides) {
    auto client = GetClientFactory().MakeClient<sample::ugrpc::UnitTestServiceClient>(MakeClientSettingsWithQos());
    ExtendDynamicConfig(MakeQosConfig(std::chrono::milliseconds{1}));

    ugrpc::client::CallOptions call_options;
    call_options.SetTimeout(utest::kMaxTestWaitTime);

    sample::ugrpc::GreetingResponse response;
    UEXPECT_NO_THROW(response = client.SayHello(kRequest, std::move(call_options)));
    EXPECT_EQ(response.name(), "Hello " + kRequest.name());
}

UTEST_F(GrpcClientQosConfigOk, UserQosOverridesEverything) {
    auto client = GetClientFactory().MakeClient<sample::ugrpc::UnitTestServiceClient>(MakeClientSettingsWithQos());
    ExtendDynamicConfig(MakeQosConfig(std::chrono::milliseconds{1}));

    ugrpc::client::CallOptions call_options;
    call_options.SetTimeout(tests::kLongTimeout);

    sample::ugrpc::GreetingResponse response;
    UEXPECT_NO_THROW(response = client.SayHello(kRequest, std::move(call_options)));
    EXPECT_EQ(response.name(), "Hello " + kRequest.name());
}

UTEST_F(GrpcClientQosConfigExceeded, DefaultTimeout) {
    auto client = GetClientFactory().MakeClient<sample::ugrpc::UnitTestServiceClient>(MakeClientSettingsWithQos());

    ExtendDynamicConfig(MakeQosConfig(tests::kShortTimeout));

    sample::ugrpc::GreetingResponse response;
    UEXPECT_THROW(response = client.SayHello(kRequest), ugrpc::client::DeadlineExceededError);
}

UTEST_F(GrpcClientQosConfigExceeded, PerRpcTimeout) {
    auto client = GetClientFactory().MakeClient<sample::ugrpc::UnitTestServiceClient>(MakeClientSettingsWithQos());

    ExtendDynamicConfig(MakePerRpcQosConfig(tests::kShortTimeout));

    sample::ugrpc::GreetingResponse response;
    UEXPECT_THROW(response = client.SayHello(kRequest), ugrpc::client::DeadlineExceededError);
}

UTEST_F(GrpcClientQosConfigExceeded, DeadlinePropagationWorks) {
    auto client = GetClientFactory().MakeClient<sample::ugrpc::UnitTestServiceClient>(MakeClientSettingsWithQos());

    ExtendDynamicConfig(MakeQosConfig(utest::kMaxTestWaitTime));
    tests::InitTaskInheritedDeadline(engine::Deadline::FromDuration(tests::kShortTimeout));

    sample::ugrpc::GreetingResponse response;
    UEXPECT_THROW(response = client.SayHello(kRequest), ugrpc::client::DeadlineExceededError);
}

UTEST_F(GrpcClientQosConfigExceeded, ContextDeadlineOverrides) {
    auto client = GetClientFactory().MakeClient<sample::ugrpc::UnitTestServiceClient>(MakeClientSettingsWithQos());

    ExtendDynamicConfig(MakeQosConfig(utest::kMaxTestWaitTime));

    ugrpc::client::CallOptions call_options;
    call_options.SetTimeout(tests::kShortTimeout);

    sample::ugrpc::GreetingResponse response;
    UEXPECT_THROW(response = client.SayHello(kRequest, std::move(call_options)), ugrpc::client::DeadlineExceededError);
}

UTEST_F(GrpcClientQosConfigExceeded, UserQosOverridesEverything) {
    auto client = GetClientFactory().MakeClient<sample::ugrpc::UnitTestServiceClient>(MakeClientSettingsWithQos());

    ExtendDynamicConfig(MakeQosConfig(utest::kMaxTestWaitTime));

    ugrpc::client::CallOptions call_options;
    call_options.SetTimeout(tests::kShortTimeout);

    sample::ugrpc::GreetingResponse response;
    UEXPECT_THROW(response = client.SayHello(kRequest, std::move(call_options)), ugrpc::client::DeadlineExceededError);
}

UTEST_F(GrpcClientQosConfigExceeded, EmptyConfigMeansInfinity) {
    auto client = GetClientFactory().MakeClient<sample::ugrpc::UnitTestServiceClient>(MakeClientSettingsWithQos());

    // Note: __default__ is not defined.
    ExtendDynamicConfig({{tests::kUnitTestClientQos, {}}});
    tests::InitTaskInheritedDeadline(engine::Deadline::FromDuration(tests::kShortTimeout));

    sample::ugrpc::GreetingResponse response;
    UEXPECT_THROW(response = client.SayHello(kRequest), ugrpc::client::DeadlineExceededError);
}

using ClientQosValidationTest = utest::LogCaptureFixture<GrpcClientQosConfigOk>;

UTEST_F(ClientQosValidationTest, ValidRpcPaths) {
    const auto client_qos = MakeClientQosWithMethods({
        "sample.ugrpc.UnitTestService/SayHello",  // Valid method for our service
        "other.service.Service/Method",           // Valid format, different service - ignored
    });
    auto client = GetClientFactory().MakeClient<sample::ugrpc::UnitTestServiceClient>(MakeClientSettingsWithQos());

    EXPECT_EQ(GetStatistics("alerts").SingleMetric(std::string{kClientQosInvalidConfigurationPath}).AsInt(), 0);

    ExtendDynamicConfig({{tests::kUnitTestClientQos, client_qos}});
    UEXPECT_NO_THROW(client.SayHello(kRequest));

    EXPECT_TRUE(GetLogCapture().Filter("Invalid RPC method path format").empty());
    EXPECT_EQ(GetStatistics("alerts").SingleMetric(std::string{kClientQosInvalidConfigurationPath}).AsInt(), 0);
}

UTEST_F(ClientQosValidationTest, InvalidRpcPathsProduceWarningsAndAlert) {
    const auto client_qos = MakeClientQosWithMethods({
        "/InvalidLeadingSlash",  // Invalid: leading slash
        "NoSlashAtAll",          // Invalid: no slash
    });
    auto client = GetClientFactory().MakeClient<sample::ugrpc::UnitTestServiceClient>(MakeClientSettingsWithQos());

    EXPECT_EQ(GetStatistics("alerts").SingleMetric(std::string{kClientQosInvalidConfigurationPath}).AsInt(), 0);

    ExtendDynamicConfig({{tests::kUnitTestClientQos, client_qos}});
    UEXPECT_NO_THROW(client.SayHello(kRequest));

    EXPECT_GE(GetLogCapture().Filter("Invalid RPC method path format").size(), 2);
    EXPECT_EQ(GetStatistics("alerts").SingleMetric(std::string{kClientQosInvalidConfigurationPath}).AsInt(), 1);

    ExtendDynamicConfig({{tests::kUnitTestClientQos, MakeClientQosWithMethods({})}});

    EXPECT_GE(GetLogCapture().Filter("Invalid RPC method path format").size(), 2);
    EXPECT_EQ(GetStatistics("alerts").SingleMetric(std::string{kClientQosInvalidConfigurationPath}).AsInt(), 0);
}

UTEST_F(ClientQosValidationTest, NonExistentMethodProducesWarningAndAlert) {
    const auto client_qos = MakeClientQosWithMethods({
        "sample.ugrpc.UnitTestService/SayHello",           // exists
        "sample.ugrpc.UnitTestService/NonExistentMethod",  // doesn't exist
    });
    auto client = GetClientFactory().MakeClient<sample::ugrpc::UnitTestServiceClient>(MakeClientSettingsWithQos());

    EXPECT_EQ(GetStatistics("alerts").SingleMetric(std::string{kClientQosInvalidConfigurationPath}).AsInt(), 0);

    ExtendDynamicConfig({{tests::kUnitTestClientQos, client_qos}});
    UEXPECT_NO_THROW(client.SayHello(kRequest));

    EXPECT_GE(GetLogCapture().Filter("does not exist in service").size(), 1);
    EXPECT_EQ(GetStatistics("alerts").SingleMetric(std::string{kClientQosInvalidConfigurationPath}).AsInt(), 1);

    ExtendDynamicConfig({{tests::kUnitTestClientQos, MakeClientQosWithMethods({})}});

    EXPECT_GE(GetLogCapture().Filter("does not exist in service").size(), 1);
    EXPECT_EQ(GetStatistics("alerts").SingleMetric(std::string{kClientQosInvalidConfigurationPath}).AsInt(), 0);
}

UTEST_F(ClientQosValidationTest, InvalidRpcPathsProduceDetailedWarningsAndAlert) {
    const auto client_qos = MakeClientQosWithMethods({
        "/InvalidLeadingSlash",  // Invalid: leading slash
        "NoSlashAtAll",          // Invalid: no slash
        "Invalid.Service/",      // Invalid: empty method name
    });
    auto client = GetClientFactory().MakeClient<sample::ugrpc::UnitTestServiceClient>(MakeClientSettingsWithQos());

    EXPECT_EQ(GetStatistics("alerts").SingleMetric(std::string{kClientQosInvalidConfigurationPath}).AsInt(), 0);

    ExtendDynamicConfig({{tests::kUnitTestClientQos, client_qos}});
    UEXPECT_NO_THROW(client.SayHello(kRequest));

    const auto warnings = GetLogCapture().Filter("Invalid RPC method path format");
    ASSERT_EQ(warnings.size(), 3);

    const auto contains_reason = [&warnings](std::string_view reason) {
        return boost::algorithm::any_of(warnings, [reason](const auto& w) {
            return w.GetText().find(reason) != std::string::npos;
        });
    };

    EXPECT_TRUE(contains_reason("Reason: Path should not start with a '/'"));
    EXPECT_TRUE(contains_reason("Reason: Path must contain exactly one '/' separator"));
    EXPECT_TRUE(contains_reason("Reason: Method name part must not be empty"));

    EXPECT_EQ(GetStatistics("alerts").SingleMetric(std::string{kClientQosInvalidConfigurationPath}).AsInt(), 1);

    ExtendDynamicConfig({{tests::kUnitTestClientQos, MakeClientQosWithMethods({})}});

    EXPECT_GE(GetLogCapture().Filter("Invalid RPC method path format").size(), 3);
    EXPECT_EQ(GetStatistics("alerts").SingleMetric(std::string{kClientQosInvalidConfigurationPath}).AsInt(), 0);
}

USERVER_NAMESPACE_END
