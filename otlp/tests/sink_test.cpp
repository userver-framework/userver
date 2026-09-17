#include <userver/utest/utest.hpp>

#include <memory>
#include <string_view>
#include <utility>
#include <variant>

#include <opentelemetry/proto/collector/logs/v1/logs_service_client.usrv.pb.hpp>
#include <opentelemetry/proto/collector/trace/v1/trace_service_client.usrv.pb.hpp>

#include <otlp/logs/logger.hpp>
#include <userver/logging/impl/formatters/base.hpp>
#include <userver/logging/log_helper.hpp>
#include <userver/ugrpc/tests/service.hpp>
#include <userver/utils/impl/source_location.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

// Nothing is exported here, so the server registers no OTLP services: the tests only inspect what the formatter builds
// NOLINTNEXTLINE(fuchsia-multiple-inheritance)
class OtlpSinkTest : public ugrpc::tests::ServiceBase, public ::testing::Test {
public:
    OtlpSinkTest() { StartServer(); }

    ~OtlpSinkTest() override { StopServer(); }

    std::shared_ptr<otlp::Logger> MakeLogger(otlp::SinkType logs_sink, otlp::SinkType tracing_sink) {
        otlp::LoggerConfig config;
        config.logs_sink = logs_sink;
        config.tracing_sink = tracing_sink;
        return std::make_shared<otlp::Logger>(
            MakeClient<opentelemetry::proto::collector::logs::v1::LogsServiceClient>(),
            MakeClient<opentelemetry::proto::collector::trace::v1::TraceServiceClient>(),
            std::move(config)
        );
    }

    static logging::impl::formatters::BasePtr MakeFormatter(otlp::Logger& logger, logging::LogClass log_class) {
        return logger.MakeFormatter(logging::Level::kInfo, log_class, utils::impl::SourceLocation::Current());
    }

    static otlp::Item& BuildItem(logging::impl::formatters::Base& formatter) {
        formatter.AddTag("key", std::string_view{"value"});
        formatter.SetText("text");
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
        return static_cast<otlp::Item&>(formatter.ExtractLoggerItem());
    }
};

}  // namespace

UTEST_F(OtlpSinkTest, DefaultLogsSinkBuildsNoOtlpRecord) {
    auto logger = MakeLogger(otlp::SinkType::kDefault, otlp::SinkType::kOtlp);

    auto formatter = MakeFormatter(*logger, logging::LogClass::kLog);
    EXPECT_TRUE(std::holds_alternative<std::monostate>(BuildItem(*formatter).otlp));

    logger->Stop();
}

UTEST_F(OtlpSinkTest, DefaultTracingSinkBuildsNoOtlpSpan) {
    auto logger = MakeLogger(otlp::SinkType::kOtlp, otlp::SinkType::kDefault);

    auto formatter = MakeFormatter(*logger, logging::LogClass::kTrace);
    EXPECT_TRUE(std::holds_alternative<std::monostate>(BuildItem(*formatter).otlp));

    logger->Stop();
}

UTEST_F(OtlpSinkTest, OtlpSinksBuildRecordAndSpan) {
    auto logger = MakeLogger(otlp::SinkType::kOtlp, otlp::SinkType::kOtlp);

    auto log_formatter = MakeFormatter(*logger, logging::LogClass::kLog);
    EXPECT_TRUE(std::holds_alternative<::opentelemetry::proto::logs::v1::LogRecord>(BuildItem(*log_formatter).otlp));

    auto trace_formatter = MakeFormatter(*logger, logging::LogClass::kTrace);
    EXPECT_TRUE(std::holds_alternative<::opentelemetry::proto::trace::v1::Span>(BuildItem(*trace_formatter).otlp));

    logger->Stop();
}

USERVER_NAMESPACE_END
