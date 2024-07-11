#pragma once

#include <memory>
#include <variant>

#include <opentelemetry/proto/collector/logs/v1/logs_service_client.usrv.pb.hpp>
#include <opentelemetry/proto/collector/trace/v1/trace_service_client.usrv.pb.hpp>

#include <userver/concurrent/queue.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/logging/impl/logger_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace otlp {

struct LoggerConfig {
  size_t max_queue_size{10000};
  std::chrono::milliseconds max_batch_delay{};

  std::string service_name;
  std::unordered_map<std::string, std::string> attributes;
  logging::Level log_level{logging::Level::kInfo};
};

class Logger final : public logging::impl::LoggerBase {
 public:
  explicit Logger(
      std::shared_ptr<
          opentelemetry::proto::collector::logs::v1::LogsServiceClient>
          client,
      std::shared_ptr<
          opentelemetry::proto::collector::trace::v1::TraceServiceClient>
          trace_client,
      LoggerConfig&& config);

  ~Logger() override;

  void Log(logging::Level level, std::string_view msg) override;

  void PrependCommonTags(logging::impl::TagWriter writer) const override;

 protected:
  bool DoShouldLog(logging::Level level) const noexcept override;

 private:
  using Action = std::variant<::opentelemetry::proto::logs::v1::LogRecord,
                              ::opentelemetry::proto::trace::v1::Span>;
  using Queue = concurrent::NonFifoMpscQueue<Action>;

  void SendingLoop(Queue::Consumer& consumer);

  bool IsTracingEntry(const std::vector<std::string_view>& key_values) const;

  void HandleTracing(const std::vector<std::string_view>& key_values);

  void HandleLog(const std::vector<std::string_view>& key_values);

  void FillAttributes(::opentelemetry::proto::resource::v1::Resource& resource);

  void DoLog(
      const opentelemetry::proto::collector::logs::v1::ExportLogsServiceRequest&
          request);

  void DoTrace(const opentelemetry::proto::collector::trace::v1::
                   ExportTraceServiceRequest& request);

  std::shared_ptr<opentelemetry::proto::collector::logs::v1::LogsServiceClient>
      client_;
  std::shared_ptr<
      opentelemetry::proto::collector::trace::v1::TraceServiceClient>
      trace_client_;
  const LoggerConfig config_;
  std::shared_ptr<Queue> queue_;
  Queue::MultiProducer queue_producer_;
  engine::Task sender_task_;  // Must be the last member
};

}  // namespace otlp

USERVER_NAMESPACE_END
