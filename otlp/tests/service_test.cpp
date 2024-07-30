#include <userver/utest/utest.hpp>

#include <vector>

#include <otlp/logs/logger.hpp>
#include <userver/engine/sleep.hpp>

#include <userver/ugrpc/tests/service_fixtures.hpp>
#include <userver/utest/default_logger_fixture.hpp>

#include <opentelemetry/proto/collector/logs/v1/logs_service_client.usrv.pb.hpp>
#include <opentelemetry/proto/collector/logs/v1/logs_service_service.usrv.pb.hpp>
#include <opentelemetry/proto/collector/trace/v1/trace_service_service.usrv.pb.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

template <typename GrpcService1, typename GrpcService2>
class Service : public ugrpc::tests::ServiceBase {
 public:
  explicit Service(ugrpc::server::ServerConfig&& server_config)
      : ServiceBase(std::move(server_config)), service1_(), service2_() {
    RegisterService(service1_);
    RegisterService(service2_);
    StartServer();
  }

  ~Service() override { StopServer(); }

  /// @returns the stored service.
  GrpcService1& GetService1() { return service1_; }
  GrpcService2& GetService2() { return service2_; }

 private:
  GrpcService1 service1_{};
  GrpcService2 service2_{};
};

class LogService final
    : public opentelemetry::proto::collector::logs::v1::LogsServiceBase {
 public:
  void Export(
      ExportCall& call,
      ::opentelemetry::proto::collector::logs::v1::ExportLogsServiceRequest&&
          request) override {
    // Don't emit new traces to avoid recursive traces/logs
    tracing::Span::CurrentSpan().SetLogLevel(logging::Level::kNone);

    for (const auto& rl : request.resource_logs()) {
      for (const auto& sl : rl.scope_logs()) {
        for (const auto& lr : sl.log_records()) {
          logs.push_back(lr);
        }
      }
    }

    call.Finish({});
  }

  // no sync as there is only a single grpc client
  std::vector<::opentelemetry::proto::logs::v1::LogRecord> logs;
};

class TraceService final
    : public opentelemetry::proto::collector::trace::v1::TraceServiceBase {
 public:
  void Export(
      ExportCall& call,
      ::opentelemetry::proto::collector::trace::v1::ExportTraceServiceRequest&&)
      override {
    // Don't emit new traces to avoid recursive traces/logs
    tracing::Span::CurrentSpan().SetLogLevel(logging::Level::kNone);

    call.Finish({});
  }
};

// NOLINTNEXTLINE(fuchsia-multiple-inheritance)
class LogServiceTest : public Service<LogService, TraceService>,
                       public utest::DefaultLoggerFixture<::testing::Test> {
 public:
  LogServiceTest() : Service({}) {
    logger_ = std::make_shared<otlp::Logger>(
        MakeClient<
            opentelemetry::proto::collector::logs::v1::LogsServiceClient>(),
        MakeClient<
            opentelemetry::proto::collector::trace::v1::TraceServiceClient>(),
        otlp::LoggerConfig{});
    SetDefaultLogger(logger_);
  }

  ~LogServiceTest() override { logger_->Stop(); }

 private:
  std::shared_ptr<otlp::Logger> logger_;
};

}  // namespace

UTEST_F(LogServiceTest, DISABLED_NoInfiniteLogsInTrace) {
  LOG_INFO() << "dummy log";

  engine::SleepFor(std::chrono::seconds(1));
  EXPECT_EQ(GetService1().logs.size(), 1);
}

UTEST_F(LogServiceTest, DISABLED_Smoke) {
  auto timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
      std::chrono::system_clock::now().time_since_epoch());
  LOG_INFO() << "log";

  while (GetService1().logs.size() < 1) {
    engine::SleepFor(std::chrono::milliseconds(10));
  }

  auto& log = GetService1().logs[0];

  EXPECT_EQ(log.body().string_value(), "log");
  EXPECT_EQ(log.severity_text(), "INFO");

  EXPECT_LE(timestamp.count(), log.time_unix_nano());
  EXPECT_LE(log.time_unix_nano(), timestamp.count() + 1'000'000'000)
      << log.time_unix_nano() - timestamp.count() - 1'000'000'000;
}

USERVER_NAMESPACE_END
