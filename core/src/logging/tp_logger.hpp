#pragma once

#include <future>
#include <memory>
#include <variant>

#include <userver/concurrent/queue.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/future.hpp>
#include <userver/logging/format.hpp>
#include <userver/logging/impl/logger_base.hpp>

#include <logging/impl/base_sink.hpp>
#include <logging/statistics/log_stats.hpp>

#include "config.hpp"

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

using SinkPtr = std::shared_ptr<BaseSink>;

namespace async {

struct Log {
  Level level{};
  std::string payload{};
  std::chrono::system_clock::time_point time{std::chrono::system_clock::now()};
};

struct FlushCoro {
  engine::Promise<void> promise;
};

struct FlushThreaded {
  std::promise<void> promise;
};

struct Stop {};

using Action = std::variant<Stop, Log, FlushCoro, FlushThreaded>;

}  // namespace async

/// @brief Asynchronous logger that logs into a specific TaskProcessor.
class TpLogger final : public LoggerBase {
 public:
  TpLogger(Format format, std::string logger_name);
  ~TpLogger() override;

  void StartAsync(engine::TaskProcessor& task_processor,
                  std::size_t max_queue_size,
                  LoggerConfig::QueueOverflowBehavior overflow_policy);

  void SwitchToSyncMode();

  void Log(Level level, std::string_view msg) const override;
  void Flush() const override;

  void SetPattern(std::string pattern);
  void AddSink(impl::SinkPtr&& sink);
  const std::vector<impl::SinkPtr>& GetSinks() const;

  std::string_view GetLoggerName() const noexcept;

  statistics::LogStatistics& GetStatistics() noexcept;

 private:
  using Queue = concurrent::NonFifoMpmcQueue<impl::async::Action>;

  struct ActionVisitor;

  void ProcessingLoop();
  bool KeepProcessing(impl::async::Action&& action) noexcept;
  void Push(impl::async::Log&& action) const;
  void BackendLog(impl::async::Log&& action) const;
  void BackendFlush() const;

  const std::string logger_name_;
  std::string formatter_pattern_;
  const std::shared_ptr<Queue> queue_;
  engine::Task consuming_task_;
  std::atomic<bool> in_async_mode_{false};
  Queue::MultiProducer producer_;
  mutable std::atomic<std::size_t> pending_async_ops_{0};
  LoggerConfig::QueueOverflowBehavior overflow_policy_{};
  std::vector<impl::SinkPtr> sinks_;
  mutable statistics::LogStatistics stats_{};
};

}  // namespace logging::impl

USERVER_NAMESPACE_END
