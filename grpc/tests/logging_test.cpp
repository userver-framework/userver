#include <userver/utest/utest.hpp>

#include <gmock/gmock.h>
#include <grpcpp/grpcpp.h>
#include <boost/utility/base_from_member.hpp>

#include <userver/utest/log_capture_fixture.hpp>
#include <userver/utils/regex.hpp>

#include <tests/unit_test_client.usrv.pb.hpp>
#include <tests/unit_test_service.usrv.pb.hpp>
#include <userver/ugrpc/tests/service_fixtures.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

ugrpc::server::ServerConfig MakeServerConfig(logging::LoggerPtr access_tskv_logger) {
    ugrpc::server::ServerConfig config;
    config.port = 0;
    config.access_tskv_logger = access_tskv_logger;
    return config;
}

class UnitTestService final : public sample::ugrpc::UnitTestServiceBase {
public:
    void SayHello(SayHelloCall& call, sample::ugrpc::GreetingRequest&& request) override {
        sample::ugrpc::GreetingResponse response;
        response.set_name("Hello " + request.name());
        call.Finish(response);
    }
};

template <typename GrpcService>
// NOLINTNEXTLINE(fuchsia-multiple-inheritance)
class ServiceWithAccessLogFixture : protected boost::base_from_member<utest::LogCaptureLogger>,
                                    public ugrpc::tests::Service<GrpcService>,
                                    public ::testing::Test {
public:
    ServiceWithAccessLogFixture()
        : boost::base_from_member<utest::LogCaptureLogger>(logging::Format::kRaw),
          ugrpc::tests::Service<GrpcService>(MakeServerConfig(member.GetLogger())) {}
};

}  // namespace

using GrpcAccessLog = ServiceWithAccessLogFixture<UnitTestService>;

UTEST_F(GrpcAccessLog, Test) {
    auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
    sample::ugrpc::GreetingRequest out;
    out.set_name("userver");
    auto response = client.SayHello(out).Finish();

    GetServer().StopServing();

    constexpr std::string_view kExpectedPattern = R"(tskv\t)"
                                                  R"(timestamp=\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}\t)"
                                                  R"(timezone=[-+]\d{2}\d{2}\t)"
                                                  R"(user_agent=.+\t)"
                                                  R"(ip=[.0-9a-f:\[\]]+\:[0-9]+\t)"
                                                  R"(x_real_ip=[.0-9a-f:\[\]]+\:[0-9]+\t)"
                                                  R"(request=[a-zA-Z./0-9]+\t)"
                                                  R"(upstream_response_time_ms=\d+\.\d+\t)"
                                                  R"(grpc_status=\d+\t)"
                                                  R"(grpc_status_code=[A-Z_]+\n)";

    const auto logs = GetSingleLog(member.GetAll());
    EXPECT_TRUE(utils::regex_match(logs.GetLogRaw(), utils::regex(kExpectedPattern))) << logs;
}

USERVER_NAMESPACE_END
