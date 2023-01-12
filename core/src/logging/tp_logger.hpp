#pragma once

/// @copybrief logging::TpLogger

#include <spdlog/logger.h>

#include <userver/concurrent/queue.hpp>
#include <userver/engine/async.hpp>

#include "config.hpp"

USERVER_NAMESPACE_BEGIN

namespace logging {

namespace impl {

enum class ActionType {
  kLog,
  kFlush,
  kStop,
};

class AsyncAction final : public spdlog::details::log_msg_buffer {
 public:
  AsyncAction() = default;

  AsyncAction(const AsyncAction&) = delete;
  AsyncAction& operator=(const AsyncAction&) = delete;
  AsyncAction(AsyncAction&&) = default;
  AsyncAction& operator=(AsyncAction&&) = default;

  ~AsyncAction() = default;

  explicit AsyncAction(ActionType type) : action_type_{type} {}

  AsyncAction(ActionType type, const spdlog::details::log_msg& m)
      : log_msg_buffer{m}, action_type_{type} {}

  ActionType GetType() const noexcept { return action_type_; }

 private:
  ActionType action_type_{ActionType::kLog};
};

}  // namespace impl

/// @brief Asynchronous logger that logs into a specific TaskProcessor.
class TpLogger final : public spdlog::logger {
 public:
  TpLogger(std::string logger_name, engine::TaskProcessor& task_processor,
           std::size_t max_queue_size,
           LoggerConfig::QueueOveflowBehavior overflow_policy);
  ~TpLogger() override;

  void SwitchToSyncMode();

 private:
  using Queue = concurrent::NonFifoMpmcQueue<impl::AsyncAction>;

  std::shared_ptr<logger> clone(std::string new_name) override;
  void sink_it_(const spdlog::details::log_msg& msg) override;
  void flush_() override;

  void ProcessingLoop();
  bool KeepProcessing(const impl::AsyncAction& action);
  void Push(impl::AsyncAction&& action);
  void BackendSinkIt(const spdlog::details::log_msg& incoming_log_msg);
  void BackendFlush();

  const std::shared_ptr<Queue> queue_;
  engine::Task consuming_task_;
  std::atomic<bool> in_fallback_mode_{false};
  Queue::MultiProducer producer_;
  const LoggerConfig::QueueOveflowBehavior overflow_policy_;
  std::atomic<std::int32_t> flush_index_{0};
};

}  // namespace logging

USERVER_NAMESPACE_END
