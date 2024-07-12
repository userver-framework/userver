#include "logger.hpp"

#include <chrono>
#include <iostream>

#include <userver/engine/async.hpp>
#include <userver/logging/impl/tag_writer.hpp>
#include <userver/logging/logger.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/encoding/hex.hpp>
#include <userver/utils/overloaded.hpp>
#include <userver/utils/text_light.hpp>

USERVER_NAMESPACE_BEGIN

namespace otlp {

namespace {
constexpr std::string_view kTelemetrySdkLanguage = "telemetry.sdk.language";
constexpr std::string_view kTelemetrySdkName = "telemetry.sdk.name";
constexpr std::string_view kServiceName = "service.name";

constexpr std::string_view kStopwatchNameEq = "stopwatch_name=";

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

void Logger::Stop() noexcept { sender_task_.SyncCancel(); }

void Logger::Log(logging::Level, std::string_view msg) {
  // Trim trailing \n
  if (msg.size() > 1 && msg[msg.size() - 1] == '\n') {
    msg = msg.substr(0, msg.size() - 1);
  }

  std::vector<std::string_view> key_values =
      utils::text::SplitIntoStringViewVector(msg, "\t");

  if (IsTracingEntry(key_values)) {
    HandleTracing(key_values);
  } else {
    HandleLog(key_values);
  }
}

void Logger::PrependCommonTags(logging::impl::TagWriter writer) const {
  logging::impl::default_::PrependCommonTags(writer);
}

bool Logger::DoShouldLog(logging::Level level) const noexcept {
  return logging::impl::default_::DoShouldLog(level);
}

bool Logger::IsTracingEntry(
    const std::vector<std::string_view>& key_values) const {
  for (const auto& key_value : key_values) {
    if (key_value.substr(0, kStopwatchNameEq.size()) == kStopwatchNameEq)
      return true;
  }
  return false;
}

void Logger::HandleLog(const std::vector<std::string_view>& key_values) {
  ::opentelemetry::proto::logs::v1::LogRecord log_records;
  std::string text;
  std::string trace_id;
  std::string span_id;
  std::string level;
  std::chrono::system_clock::time_point timestamp;

  for (const auto key_value : key_values) {
    auto eq_pos = key_value.find('=');
    if (eq_pos == std::string::npos) continue;

    auto key = key_value.substr(0, eq_pos);
    auto value = key_value.substr(eq_pos + 1);

    if (key == "text") {
      text = std::string{value};
      continue;
    }
    if (key == "trace_id") {
      trace_id = utils::encoding::FromHex(value);
      continue;
    }
    if (key == "span_id") {
      span_id = utils::encoding::FromHex(value);
      continue;
    }
    if (key == "timestamp") {
      timestamp = utils::datetime::Stringtime(std::string{value},
                                              utils::datetime::kDefaultTimezone,
                                              kTimestampFormat);
      continue;
    }
    if (key == "level") {
      level = value;
      continue;
    }

    auto attributes = log_records.add_attributes();
    attributes->set_key(std::string{key});
    attributes->mutable_value()->set_string_value(std::string{value});
  }

  log_records.set_severity_text(grpc::string(level));
  log_records.mutable_body()->set_string_value(grpc::string(text));
  log_records.set_trace_id(std::move(trace_id));
  log_records.set_span_id(std::move(span_id));

  auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(
      timestamp.time_since_epoch());
  log_records.set_time_unix_nano(nanoseconds.count());

  // Drop a log if overflown
  [[maybe_unused]] auto ok =
      queue_producer_.PushNoblock(std::move(log_records));

  // TODO: count drops
}

void Logger::HandleTracing(const std::vector<std::string_view>& key_values) {
  ::opentelemetry::proto::trace::v1::Span span;
  std::string trace_id;
  std::string span_id;
  std::string parent_span_id;
  std::string_view level;
  std::string_view stopwatch_name;

  std::string start_timestamp;
  std::string total_time;

  for (const auto key_value : key_values) {
    auto eq_pos = key_value.find('=');
    if (eq_pos == std::string::npos) continue;

    auto key = key_value.substr(0, eq_pos);
    auto value = key_value.substr(eq_pos + 1);

    if (key == "trace_id") {
      trace_id = utils::encoding::FromHex(value);
      continue;
    }
    if (key == "span_id") {
      span_id = utils::encoding::FromHex(value);
      continue;
    }
    if (key == "parent_span_id") {
      parent_span_id = utils::encoding::FromHex(value);
      continue;
    }
    if (key == "stopwatch_name") {
      stopwatch_name = value;
      continue;
    }
    if (key == "total_time") {
      total_time = value;
      continue;
    }
    if (key == "start_timestamp") {
      start_timestamp = value;
      continue;
    }
    if (key == "level") {
      level = value;
      continue;
    }
    if (key == "timestamp" || key == "text") {
      continue;
    }

    auto attributes = span.add_attributes();
    attributes->set_key(std::string{key});
    attributes->mutable_value()->set_string_value(std::string{value});
  }

  span.set_trace_id(std::string(trace_id));
  span.set_span_id(std::string(span_id));
  span.set_parent_span_id(std::string(parent_span_id));
  span.set_name(std::string(stopwatch_name));

  auto start_timestamp_double = std::stod(start_timestamp);
  span.set_start_time_unix_nano(start_timestamp_double * 1'000'000'000);
  span.set_end_time_unix_nano((start_timestamp_double + std::stod(total_time)) *
                              1'000'000'000);

  // Drop a trace if overflown
  [[maybe_unused]] auto ok = queue_producer_.PushNoblock(std::move(span));

  // TODO: count drops
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

  for (const auto& [key, value] : config_.attributes) {
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

}  // namespace otlp

USERVER_NAMESPACE_END
