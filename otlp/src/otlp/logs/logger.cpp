#include "logger.hpp"

#include <chrono>

#include <userver/engine/async.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/parse/to.hpp>
#include <userver/logging/impl/tag_writer.hpp>
#include <userver/logging/logger.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/encoding/hex.hpp>
#include <userver/utils/overloaded.hpp>
#include <userver/utils/text_light.hpp>
#include <userver/utils/underlying_value.hpp>

USERVER_NAMESPACE_BEGIN

namespace otlp {

namespace {
constexpr std::string_view kTelemetrySdkLanguage = "telemetry.sdk.language";
constexpr std::string_view kTelemetrySdkName = "telemetry.sdk.name";
constexpr std::string_view kServiceName = "service.name";

const std::string kTimestampFormat = "%Y-%m-%dT%H:%M:%E*S";
}  // namespace

Formatter::Formatter(
    logging::Level level,
    logging::LogClass log_class,
    SinkType sink_type,
    logging::LoggerPtr default_logger,
    Logger& logger
)
    : logger_(logger) {
    if (sink_type == SinkType::kOtlp || sink_type == SinkType::kBoth) {
        if (log_class == logging::LogClass::kLog) {
            auto& log_record = item_.otlp.emplace<::opentelemetry::proto::logs::v1::LogRecord>();
            log_record.set_severity_text(grpc::string{logging::ToUpperCaseString(level)});

            auto nanoseconds =
                std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()
                );
            log_record.set_time_unix_nano(nanoseconds.count());
        } else {
            item_.otlp.emplace<::opentelemetry::proto::trace::v1::Span>();
        }
    }

    if (sink_type == SinkType::kDefault || sink_type == SinkType::kBoth) {
        if (default_logger) {
            item_.forwarded_formatter = default_logger->MakeFormatter(level, log_class);
        }
    }
}

void Formatter::AddTag(std::string_view key, const logging::LogExtra::Value& value) {
    std::visit(
        utils::Overloaded{
            [&](opentelemetry::proto::trace::v1::Span& span) {
                if (key == "trace_id") {
                    span.set_trace_id(utils::encoding::FromHex(std::get<std::string>(value)));
                } else if (key == "span_id") {
                    span.set_span_id(utils::encoding::FromHex(std::get<std::string>(value)));
                } else if (key == "parent_id") {
                    span.set_parent_span_id(utils::encoding::FromHex(std::get<std::string>(value)));
                } else if (key == "stopwatch_name") {
                    span.set_name(std::get<std::string>(value));
                } else if (key == "total_time") {
                    item_.total_time = std::get<double>(value);
                } else if (key == "start_timestamp") {
                    auto start_timestamp_str = std::get<std::string>(value);
                    item_.start_timestamp = std::stod(start_timestamp_str);
                    span.set_start_time_unix_nano(item_.start_timestamp * 1'000'000'000);
                } else if (key == "timestamp" || key == "text") {
                    // nothing
                } else {
                    auto attributes = span.add_attributes();
                    attributes->set_key(std::string{logger_.MapAttribute(key)});
                    logger_.SetAttributeValue(attributes->mutable_value(), value);
                }
            },
            [&](opentelemetry::proto::logs::v1::LogRecord& log_record) {
                if (key == "trace_id") {
                    log_record.set_trace_id(utils::encoding::FromHex(std::get<std::string>(value)));
                } else if (key == "span_id") {
                    log_record.set_span_id(utils::encoding::FromHex(std::get<std::string>(value)));
                } else {
                    auto attributes = log_record.add_attributes();
                    attributes->set_key(std::string{logger_.MapAttribute(key)});
                    logger_.SetAttributeValue(attributes->mutable_value(), value);
                }
            },
        },
        item_.otlp
    );

    if (item_.forwarded_formatter) {
        item_.forwarded_formatter->AddTag(key, value);
    }
}

void Formatter::AddTag(std::string_view key, std::string_view value) {
    AddTag(key, logging::LogExtra::Value{std::string{value}});
}

void Formatter::SetText(std::string_view text) {
    if (auto* log_record = std::get_if<::opentelemetry::proto::logs::v1::LogRecord>(&item_.otlp)) {
        log_record->mutable_body()->set_string_value(std::string(text));
    }

    if (item_.forwarded_formatter) {
        item_.forwarded_formatter->SetText(text);
    }
}

logging::impl::formatters::LoggerItemRef Formatter::ExtractLoggerItem() {
    if (auto* span = std::get_if<::opentelemetry::proto::trace::v1::Span>(&item_.otlp)) {
        span->set_end_time_unix_nano((item_.start_timestamp + item_.total_time / 1'000) * 1'000'000'000LL);
    }
    return item_;
}

SinkType Parse(const yaml_config::YamlConfig& value, formats::parse::To<SinkType>) {
    auto destination = value.As<std::string>("otlp");
    if (destination == "both") {
        return SinkType::kBoth;
    }
    if (destination == "default") {
        return SinkType::kDefault;
    }
    if (destination == "otlp") {
        return SinkType::kOtlp;
    }
    throw std::runtime_error("OTLP logger: unknown sink type:" + destination);
}

Logger::Logger(
    opentelemetry::proto::collector::logs::v1::LogsServiceClient client,
    opentelemetry::proto::collector::trace::v1::TraceServiceClient trace_client,
    LoggerConfig&& config
)
    : config_(std::move(config)),
      queue_(Queue::Create(config_.max_queue_size)),
      queue_producer_(queue_->GetMultiProducer()) {
    SetLevel(config_.log_level);
    std::cerr << "OTLP logger has started\n";

    sender_task_ = engine::CriticalAsyncNoSpan([this,
                                                consumer = queue_->GetConsumer(),
                                                log_client = std::move(client),
                                                trace_client = std::move(trace_client)]() mutable {
        SendingLoop(consumer, log_client, trace_client);
    });
    ;
}

Logger::~Logger() { Stop(); }

void Logger::Stop() noexcept {
    sender_task_.SyncCancel();
    sender_task_ = {};
}

const logging::impl::LogStatistics& Logger::GetStatistics() const { return stats_; }

void Logger::PrependCommonTags(logging::impl::TagWriter writer) const {
    logging::impl::default_::PrependCommonTags(writer);
}

bool Logger::DoShouldLog(logging::Level level) const noexcept { return logging::impl::default_::DoShouldLog(level); }

void Logger::Log(logging::Level level, logging::impl::formatters::LoggerItemRef item) {
    UASSERT(dynamic_cast<Item*>(&item));
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
    auto& log = static_cast<Item&>(item);

    if (!log.otlp.valueless_by_exception()) {
        bool ok = queue_producer_.PushNoblock(std::move(log.otlp));
        if (!ok) {
            // Drop a log/trace if overflown
            ++stats_.dropped;
        }
    }

    if (default_logger_ && log.forwarded_formatter) {
        auto& fwd_item = log.forwarded_formatter->ExtractLoggerItem();
        default_logger_->Log(level, fwd_item);
    }
}

logging::impl::formatters::BasePtr Logger::MakeFormatter(logging::Level level, logging::LogClass log_class) {
    auto sink = log_class == logging::LogClass::kLog ? config_.logs_sink : config_.tracing_sink;
    return std::make_unique<Formatter>(level, log_class, sink, default_logger_, *this);
}

void Logger::SetAttributeValue(
    ::opentelemetry::proto::common::v1::AnyValue* destination,
    const logging::LogExtra::Value& value
) {
    std::visit(
        utils::Overloaded{
            [&](bool x) { destination->set_bool_value(x); },
            [&](int x) { destination->set_int_value(x); },
            [&](long x) { destination->set_int_value(x); },
            [&](unsigned int x) { destination->set_int_value(x); },
            [&](unsigned long x) { destination->set_int_value(x); },
            [&](long long x) { destination->set_int_value(x); },
            [&](unsigned long long x) { destination->set_int_value(x); },
            [&](double x) { destination->set_double_value(x); },
            [&](const std::string& x) { destination->set_string_value(x); }},
        value
    );
}

void Logger::SendingLoop(Queue::Consumer& consumer, LogClient& log_client, TraceClient& trace_client) {
    // Create dummy span to completely disable logging in current coroutine
    tracing::Span span("");
    span.SetLocalLogLevel(logging::Level::kNone);

    ::opentelemetry::proto::collector::logs::v1::ExportLogsServiceRequest log_request;
    auto resource_logs = log_request.add_resource_logs();
    auto scope_logs = resource_logs->add_scope_logs();
    FillAttributes(*resource_logs->mutable_resource());

    ::opentelemetry::proto::collector::trace::v1::ExportTraceServiceRequest trace_request;
    auto resource_spans = trace_request.add_resource_spans();
    auto scope_spans = resource_spans->add_scope_spans();
    FillAttributes(*resource_spans->mutable_resource());

    Action action{};
    while (consumer.Pop(action)) {
        scope_logs->clear_log_records();
        scope_spans->clear_spans();

        auto deadline = engine::Deadline::FromDuration(config_.max_batch_delay);

        do {
            std::visit(
                utils::Overloaded{
                    [&scope_spans](const opentelemetry::proto::trace::v1::Span& action) {
                        auto span = scope_spans->add_spans();
                        *span = action;
                    },
                    [&scope_logs](const opentelemetry::proto::logs::v1::LogRecord& action) {
                        auto log_records = scope_logs->add_log_records();
                        *log_records = action;
                    }},
                action
            );
        } while (consumer.Pop(action, deadline));

        if (utils::UnderlyingValue(config_.logs_sink) & utils::UnderlyingValue(SinkType::kOtlp)) {
            DoLog(log_request, log_client);
        }
        if (utils::UnderlyingValue(config_.tracing_sink) & utils::UnderlyingValue(SinkType::kOtlp)) {
            DoTrace(trace_request, trace_client);
        }
    }
}

void Logger::FillAttributes(::opentelemetry::proto::resource::v1::Resource& resource) {
    {
        auto* attr = resource.add_attributes();
        attr->set_key(std::string{kTelemetrySdkLanguage});
        attr->mutable_value()->set_string_value("cpp");
    }

    {
        auto* attr = resource.add_attributes();
        attr->set_key(std::string{kTelemetrySdkName});
        attr->mutable_value()->set_string_value("userver");
    }

    {
        auto* attr = resource.add_attributes();
        attr->set_key(std::string{kServiceName});
        attr->mutable_value()->set_string_value(std::string{config_.service_name});
    }

    for (const auto& [key, value] : config_.extra_attributes) {
        auto* attr = resource.add_attributes();
        attr->set_key(std::string{key});
        attr->mutable_value()->set_string_value(std::string{value});
    }
}

void Logger::DoLog(
    const opentelemetry::proto::collector::logs::v1::ExportLogsServiceRequest& request,
    LogClient& client
) {
    try {
        auto response = client.Export(request);
    } catch (const ugrpc::client::RpcCancelledError&) {
        std::cerr << "Stopping OTLP sender task\n";
        throw;
    } catch (const std::exception& e) {
        std::cerr << "Failed to write down OTLP log(s): " << e.what() << typeid(e).name() << "\n";
    }
    // TODO: count exceptions
}

void Logger::DoTrace(
    const opentelemetry::proto::collector::trace::v1::ExportTraceServiceRequest& request,
    TraceClient& trace_client
) {
    try {
        auto response = trace_client.Export(request);
    } catch (const ugrpc::client::RpcCancelledError&) {
        std::cerr << "Stopping OTLP sender task\n";
        throw;
    } catch (const std::exception& e) {
        std::cerr << "Failed to write down OTLP trace(s): " << e.what() << typeid(e).name() << "\n";
    }
    // TODO: count exceptions
}

std::string_view Logger::MapAttribute(std::string_view attr) const {
    for (const auto& [key, value] : config_.attributes_mapping) {
        if (key == attr) return value;
    }
    return attr;
}

}  // namespace otlp

USERVER_NAMESPACE_END
