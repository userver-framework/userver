#include <userver/utest/utest.hpp>

#include <grpcpp/grpcpp.h>

#include <userver/logging/impl/logger_base.hpp>
#include <userver/utils/regex.hpp>

#include <tests/unit_test_client.usrv.pb.hpp>
#include <tests/unit_test_service.usrv.pb.hpp>
#include <userver/ugrpc/tests/service_fixtures.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class Logger final : public logging::impl::LoggerBase {
 public:
  Logger() noexcept : LoggerBase(logging::Format::kRaw) {
    LoggerBase::SetLevel(logging::Level::kInfo);
  }

  void SetLevel(logging::Level) override {}  // do nothing
  void Log(logging::Level, std::string_view str) override { log += str; }
  void Flush() override {}

  std::string log;
};

struct LoggerHolder {
  std::shared_ptr<Logger> logger_;
};

ugrpc::server::ServerConfig MakeServerConfig(
    std::shared_ptr<Logger> access_tskv_logger) {
  ugrpc::server::ServerConfig config;
  config.port = 0;
  config.access_tskv_logger = access_tskv_logger;
  return config;
}

class UnitTestService final : public sample::ugrpc::UnitTestServiceBase {
 public:
  void SayHello(SayHelloCall& call,
                sample::ugrpc::GreetingRequest&& request) override {
    sample::ugrpc::GreetingResponse response;
    response.set_name("Hello " + request.name());
    call.Finish(response);
  }
};

template <typename GrpcService>
// NOLINTNEXTLINE(fuchsia-multiple-inheritance)
class ServiceWithAccessLogFixture : public LoggerHolder,
                                    public ugrpc::tests::Service<GrpcService>,
                                    public ::testing::Test {
 public:
  ServiceWithAccessLogFixture()
      : LoggerHolder{std::make_shared<Logger>()},
        ugrpc::tests::Service<GrpcService>(
            dynamic_config::MakeDefaultStorage({}), MakeServerConfig(logger_)) {
  }
};

}  // namespace

using GrpcAccessLog = ServiceWithAccessLogFixture<UnitTestService>;

UTEST_F(GrpcAccessLog, Test) {
  auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();
  sample::ugrpc::GreetingRequest out;
  out.set_name("userver");
  auto response = client.SayHello(out).Finish();

  GetServer().StopDebug();

  constexpr std::string_view kExpectedPattern =
      R"(tskv\t)"
      R"(timestamp=\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}\t)"
      R"(timezone=[-+]\d{2}\d{2}\t)"
      R"(user_agent=.+\t)"
      R"(ip=[.0-9a-f:\[\]]+\:[0-9]+\t)"
      R"(x_real_ip=[.0-9a-f:\[\]]+\:[0-9]+\t)"
      R"(request=[a-zA-Z./0-9]+\t)"
      R"(upstream_response_time_ms=\d+\.\d+\t)"
      R"(grpc_status=\d+\t)"
      R"(grpc_status_code=[A-Z_]+\n)";

  EXPECT_TRUE(utils::regex_match(logger_->log, utils::regex(kExpectedPattern)))
      << logger_->log;
}

USERVER_NAMESPACE_END
