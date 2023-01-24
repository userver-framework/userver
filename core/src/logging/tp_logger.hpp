#pragma once

#include <memory>

#include <userver/concurrent/queue.hpp>
#include <userver/engine/async.hpp>
#include <userver/logging/format.hpp>
#include <userver/logging/impl/logger_base.hpp>

#include "config.hpp"

namespace spdlog::sinks {
class sink;
}

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

using SinkPtr = std::shared_ptr<spdlog::sinks::sink>;

enum class ActionType {
  kLog,
  kFlush,
  kStop,
};

class AsyncAction final {
 public:
  AsyncAction() = default;

  AsyncAction(const AsyncAction&) = delete;
  AsyncAction& operator=(const AsyncAction&) = delete;
  AsyncAction(AsyncAction&&) = default;
  AsyncAction& operator=(AsyncAction&&) = default;

  ~AsyncAction() = default;

  explicit AsyncAction(ActionType type);
  AsyncAction(Level level, std::string payload);

  ActionType GetType() const noexcept;
  Level GetLevel() const noexcept;
  std::chrono::system_clock::time_point GetTime() const noexcept;
  std::string_view GetPayloadView() const noexcept;

 private:
  ActionType action_type_{ActionType::kLog};
  Level level_{};
  std::chrono::system_clock::time_point time_{};
  std::string payload_{};
};

/// @brief Asynchronous logger that logs into a specific TaskProcessor.
class TpLogger final : public LoggerBase {
 public:
  TpLogger(Format format, std::string logger_name);
  ~TpLogger() override;

  void StartAsync(engine::TaskProcessor& task_processor,
                  std::size_t max_queue_size,
                  LoggerConfig::QueueOveflowBehavior overflow_policy);

  void SwitchToSyncMode();

  void Log(Level level, std::string_view msg) const override;
  void Flush() const override;

  void SetPattern(std::string pattern);
  void AddSink(impl::SinkPtr&& sink);
  const std::vector<impl::SinkPtr>& GetSinks() const;

  std::string_view GetLoggerName() const noexcept;

 private:
  using Queue = concurrent::NonFifoMpmcQueue<impl::AsyncAction>;

  void ProcessingLoop();
  bool KeepProcessing(impl::AsyncAction&& action);
  void Push(impl::AsyncAction&& action) const;
  void BackendLog(impl::AsyncAction&& action) const;
  void BackendFlush() const;

  const std::string logger_name_;
  std::string formatter_pattern_;
  const std::shared_ptr<Queue> queue_;
  engine::Task consuming_task_;
  std::atomic<bool> in_async_mode_{false};
  Queue::MultiProducer producer_;
  LoggerConfig::QueueOveflowBehavior overflow_policy_{};
  std::atomic<std::int32_t> flush_index_{0};
  std::vector<impl::SinkPtr> sinks_;
};

}  // namespace logging::impl

USERVER_NAMESPACE_END
