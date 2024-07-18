#include "logger.hpp"

#include <chrono>
#include <iostream>

#include <userver/engine/async.hpp>
#include <userver/logging/impl/tag_writer.hpp>
#include <userver/logging/logger.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/encoding/hex.hpp>
#include <userver/utils/encoding/tskv_parser_read.hpp>
#include <userver/utils/overloaded.hpp>
#include <userver/utils/text_light.hpp>

USERVER_NAMESPACE_BEGIN

namespace otlp {

namespace {
constexpr std::string_view kTelemetrySdkLanguage = "telemetry.sdk.language";
constexpr std::string_view kTelemetrySdkName = "telemetry.sdk.name";
constexpr std::string_view kServiceName = "service.name";

const std::string kTimestampFormat = "%Y-%m-%dT%H:%M:%E*S";
}  // namespace

Logger::Logger(
    opentelemetry::proto::collector::logs::v1::LogsServiceClient client,
    opentelemetry::proto::collector::trace::v1::TraceServiceClient trace_client,
    LoggerConfig&& config)
    : LoggerBase(logging::Format::kTskv),
      config_(std::move(config)),
      queue_(Queue::Create(config_.max_queue_size)),
      queue_producer_(queue_->GetMultiProducer()) {
  SetLevel(config_.log_level);
  Log(logging::Level::kInfo, "OTLP logger has started");
  std::cerr << "OTLP logger has started\n";

  sender_task_ = engine::CriticalAsyncNoSpan(
      [this, consumer = queue_->GetConsumer(), log_client = std::move(client),
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

const logging::impl::LogStatistics& Logger::GetStatistics() const {
  return stats_;
}

void Logger::PrependCommonTags(logging::impl::TagWriter writer) const {
  logging::impl::default_::PrependCommonTags(writer);
}

bool Logger::DoShouldLog(logging::Level level) const noexcept {
  return logging::impl::default_::DoShouldLog(level);
}

void Logger::Log(logging::Level level, std::string_view msg) {
  utils::encoding::TskvParser parser{msg};

  ::opentelemetry::proto::logs::v1::LogRecord log_record;
  std::chrono::system_clock::time_point timestamp;

  ++stats_.by_level[static_cast<int>(level)];

  [[maybe_unused]] auto parse_ok = utils::encoding::TskvReadRecord(
      parser, [&](std::string_view key, std::string_view value) {
        if (key == "text") {
          log_record.mutable_body()->set_string_value(
              grpc::string(std::string{value}));
          return true;
        }
        if (key == "trace_id") {
          log_record.set_trace_id(utils::encoding::FromHex(value));
          return true;
        }
        if (key == "span_id") {
          log_record.set_span_id(utils::encoding::FromHex(value));
          return true;
        }
        if (key == "timestamp") {
          timestamp = utils::datetime::Stringtime(
              std::string{value}, utils::datetime::kDefaultTimezone,
              kTimestampFormat);
          return true;
        }
        if (key == "level") {
          log_record.set_severity_text(grpc::string(std::string{value}));
          return true;
        }

        auto attributes = log_record.add_attributes();
        attributes->set_key(std::string{MapAttribute(key)});
        attributes->mutable_value()->set_string_value(std::string{value});
        return true;
      });

  auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(
      timestamp.time_since_epoch());
  log_record.set_time_unix_nano(nanoseconds.count());

  // Drop a log if overflown
  auto ok = queue_producer_.PushNoblock(std::move(log_record));
  if (!ok) {
    ++stats_.dropped;
  }
}

void Logger::Trace(logging::Level, std::string_view msg) {
  utils::encoding::TskvParser parser{msg};

  ::opentelemetry::proto::trace::v1::Span span;

  std::string start_timestamp;
  std::string total_time;

  [[maybe_unused]] auto parse_ok = utils::encoding::TskvReadRecord(
      parser, [&](std::string_view key, std::string_view value) {
        if (key == "trace_id") {
          span.set_trace_id(utils::encoding::FromHex(value));
          return true;
        }
        if (key == "span_id") {
          span.set_span_id(utils::encoding::FromHex(value));
          return true;
        }
        if (key == "parent_span_id") {
          span.set_parent_span_id(utils::encoding::FromHex(value));
          return true;
        }
        if (key == "stopwatch_name") {
          span.set_name(std::string(value));
          return true;
        }
        if (key == "total_time") {
          total_time = value;
          return true;
        }
        if (key == "start_timestamp") {
          start_timestamp = value;
          return true;
        }
        if (key == "timestamp" || key == "text") {
          return true;
        }

        auto attributes = span.add_attributes();
        attributes->set_key(std::string{MapAttribute(key)});
        attributes->mutable_value()->set_string_value(std::string{value});
        return true;
      });

  auto start_timestamp_double = std::stod(start_timestamp);
  span.set_start_time_unix_nano(start_timestamp_double * 1'000'000'000);
  span.set_end_time_unix_nano((start_timestamp_double + std::stod(total_time)) *
                              1'000'000'000);

  // Drop a trace if overflown
  auto ok = queue_producer_.PushNoblock(std::move(span));
  if (!ok) {
    ++stats_.dropped;
  }
}

void Logger::SendingLoop(Queue::Consumer& consumer, LogClient& log_client,
                         TraceClient& trace_client) {
  // Create dummy span to completely disable logging in current coroutine
  tracing::Span span("");
  span.SetLocalLogLevel(logging::Level::kNone);

  ::opentelemetry::proto::collector::logs::v1::ExportLogsServiceRequest
      log_request;
  auto resource_logs = log_request.add_resource_logs();
  auto scope_logs = resource_logs->add_scope_logs();
  FillAttributes(*resource_logs->mutable_resource());

  ::opentelemetry::proto::collector::trace::v1::ExportTraceServiceRequest
      trace_request;
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
              [&scope_spans](
                  const opentelemetry::proto::trace::v1::Span& action) {
                auto span = scope_spans->add_spans();
                *span = action;
              },
              [&scope_logs](
                  const opentelemetry::proto::logs::v1::LogRecord& action) {
                auto log_records = scope_logs->add_log_records();
                *log_records = action;
              }},
          action);
    } while (consumer.Pop(action, deadline));

    DoLog(log_request, log_client);
    DoTrace(trace_request, trace_client);
  }
}

void Logger::FillAttributes(
    ::opentelemetry::proto::resource::v1::Resource& resource) {
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
    const opentelemetry::proto::collector::logs::v1::ExportLogsServiceRequest&
        request,
    LogClient& client) {
  try {
    auto call = client.Export(request);
    auto response = call.Finish();
  } catch (const ugrpc::client::RpcCancelledError&) {
    std::cerr << "Stopping OTLP sender task\n";
    throw;
  } catch (const std::exception& e) {
    std::cerr << "Failed to write down OTLP log(s): " << e.what()
              << typeid(e).name() << "\n";
  }
  // TODO: count exceptions
}

void Logger::DoTrace(
    const opentelemetry::proto::collector::trace::v1::ExportTraceServiceRequest&
        request,
    TraceClient& trace_client) {
  try {
    auto call = trace_client.Export(request);
    auto response = call.Finish();
  } catch (const ugrpc::client::RpcCancelledError&) {
    std::cerr << "Stopping OTLP sender task\n";
    throw;
  } catch (const std::exception& e) {
    std::cerr << "Failed to write down OTLP trace(s): " << e.what()
              << typeid(e).name() << "\n";
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
