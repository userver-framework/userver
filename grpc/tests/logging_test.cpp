#include <userver/utest/utest.hpp>

#include <gmock/gmock.h>

#include <userver/utest/log_capture_fixture.hpp>
#include <userver/utils/regex.hpp>

#include <ugrpc/server/middlewares/access_log/middleware.hpp>

#include <tests/unit_test_client.usrv.pb.hpp>
#include <tests/unit_test_service.usrv.pb.hpp>
#include <userver/ugrpc/tests/service_fixtures.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr std::string_view kBaseTskvPattern = R"(tskv\t)"
                                              R"(timestamp=\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}\t)"
                                              R"(timezone=[-+]\d{2}\d{2}\t)"
                                              R"(user_agent=.+\t)"
                                              R"(ip=[.0-9a-f:\[\]]+\:[0-9]+\t)"
                                              R"(x_real_ip=[.0-9a-f:\[\]]+\:[0-9]+\t)"
                                              R"(request=[a-zA-Z./0-9]+\t)"
                                              R"(request_time=\d+\.\d+\t)"
                                              R"(upstream_response_time=\d+\.\d+\t)"
                                              R"(grpc_status=\d+\t)"
                                              R"(grpc_status_code=[A-Z_]+)";

const utils::regex kBaseTskvRegex{std::string{kBaseTskvPattern} + R"(\n)"};

// Constant to check result of LogExtra middleware
const utils::regex kTskvWithTagsRegex{
    std::string{kBaseTskvPattern} + R"(\tuser_id=12345)"
                                    R"(\tsession_id=abc-def-ghi)"
                                    R"(\trequest_id=req-98765)"
                                    R"(\tcustom_tag=test-value)"
                                    R"(\tnumeric_tag=42\n)"};

class UnitTestService final : public sample::ugrpc::UnitTestServiceBase {
public:
    SayHelloResult SayHello(CallContext& /*context*/, sample::ugrpc::GreetingRequest&& request) override {
        sample::ugrpc::GreetingResponse response;
        response.set_name("Hello " + request.name());
        return response;
    }
};

template <typename GrpcService>
class ServiceWithAccessLogFixture : public ugrpc::tests::ServiceFixtureBase {
public:
    ServiceWithAccessLogFixture() {
        ugrpc::server::middlewares::access_log::Settings access_log_settings;
        access_log_settings.access_tskv_logger = access_logger_.GetLogger();

        SetServerMiddlewares({
            std::make_shared<ugrpc::server::middlewares::access_log::Middleware>(std::move(access_log_settings)),
        });

        RegisterService(service_);
        StartServer();
    }

    ~ServiceWithAccessLogFixture() override { StopServer(); }

    utest::LogCaptureLogger& AccessLog() { return access_logger_; }

private:
    utest::LogCaptureLogger access_logger_{logging::Format::kRaw};
    GrpcService service_{};
};

// Test middleware that adds custom tags to LogExtra
class TagsMiddleware final : public ugrpc::server::MiddlewareBase {
public:
    void OnCallStart(ugrpc::server::MiddlewareCallContext& context) const override {
        auto* log_extra = context.GetStorageContext().GetOptional(ugrpc::server::middlewares::access_log::kLogExtraTag);

        if (log_extra) {
            log_extra->Extend("user_id", "12345");
            log_extra->Extend("session_id", "abc-def-ghi");
            log_extra->Extend("request_id", "req-98765");
            log_extra->Extend("custom_tag", "test-value");
            log_extra->Extend("numeric_tag", "42");
        }
    }
};

template <typename GrpcService>
class ServiceWithAccessLogTagsFixture : public ugrpc::tests::ServiceFixtureBase {
public:
    ServiceWithAccessLogTagsFixture() {
        ugrpc::server::middlewares::access_log::Settings access_log_settings;
        access_log_settings.access_tskv_logger = access_logger_.GetLogger();

        SetServerMiddlewares({
            std::make_shared<ugrpc::server::middlewares::access_log::Middleware>(std::move(access_log_settings)),
            std::make_shared<TagsMiddleware>(),
        });

        RegisterService(service_);
        StartServer();
    }

    ~ServiceWithAccessLogTagsFixture() override { StopServer(); }

    utest::LogCaptureLogger& AccessLog() { return access_logger_; }

private:
    utest::LogCaptureLogger access_logger_{logging::Format::kRaw};
    GrpcService service_{};
};

}  // namespace

using GrpcAccessLog = ServiceWithAccessLogFixture<UnitTestService>;
using GrpcAccessLogWithTags = ServiceWithAccessLogTagsFixture<UnitTestService>;

UTEST_F(GrpcAccessLog, Test) {
    auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
    sample::ugrpc::GreetingRequest out;
    out.set_name("userver");
    auto response = client.SayHello(out);

    GetServer().StopServing();

    const auto logs = GetSingleLog(AccessLog().GetAll());
    EXPECT_TRUE(utils::regex_match(logs.GetLogRaw(), kBaseTskvRegex)) << logs;
}

UTEST_F(GrpcAccessLogWithTags, TestWithCustomTags) {
    auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
    sample::ugrpc::GreetingRequest out;
    out.set_name("userver");
    auto response = client.SayHello(out);

    GetServer().StopServing();

    const auto logs = GetSingleLog(AccessLog().GetAll());
    EXPECT_TRUE(utils::regex_match(logs.GetLogRaw(), kTskvWithTagsRegex)) << logs;
}

USERVER_NAMESPACE_END
