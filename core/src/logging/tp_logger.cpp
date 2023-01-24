#include "tp_logger.hpp"

// this header must be included before any spdlog headers
// to override spdlog's level names
#include <logging/spdlog.hpp>

#include <memory>
#include <string>

#include <userver/concurrent/mpsc_queue.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/logging/log.hpp>

#include <engine/task/task_context.hpp>
#include <logging/spdlog_helpers.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

AsyncAction::AsyncAction(ActionType type) : action_type_{type} {}

AsyncAction::AsyncAction(Level level, std::string payload)
    : level_{level},
      time_{std::chrono::system_clock::now()},
      payload_{std::move(payload)} {}

ActionType AsyncAction::GetType() const noexcept { return action_type_; }

Level AsyncAction::GetLevel() const noexcept { return level_; }

std::chrono::system_clock::time_point AsyncAction::GetTime() const noexcept {
  return time_;
}

std::string_view AsyncAction::GetPayloadView() const noexcept {
  return payload_;
}

TpLogger::TpLogger(Format format, std::string logger_name)
    : LoggerBase(format),
      logger_name_(std::move(logger_name)),
      formatter_pattern_(GetSpdlogPattern(format)),
      queue_(Queue::Create(0)),
      producer_(queue_->GetMultiProducer()) {
  SetLevel(logging::Level::kInfo);
}

void TpLogger::StartAsync(engine::TaskProcessor& task_processor,
                          std::size_t max_queue_size,
                          LoggerConfig::QueueOveflowBehavior overflow_policy) {
  const auto was_async = in_async_mode_.exchange(true);
  UINVARIANT(!was_async,
             "Attempt to switch logger to async mode, while it is already in "
             "that mode");

  overflow_policy_ = overflow_policy;
  queue_->SetSoftMaxSize(max_queue_size);
  consuming_task_ = engine::CriticalAsyncNoSpan(
      task_processor, &TpLogger::ProcessingLoop, this);
}

TpLogger::~TpLogger() {
  UASSERT_MSG(
      !in_async_mode_,
      "We may be in non coroutine context, async logger must be in sync mode");
  UASSERT_MSG(!consuming_task_.IsValid(),
              "We may be in non coroutine context, async logger must be in "
              "sync mode and consuming task must be stopped");
}

void TpLogger::SwitchToSyncMode() {
  const auto was_async = in_async_mode_.exchange(false);
  if (!was_async) {
    return;
  }

  // Some loggings could be in progress. To wait for those we send a kStop
  // message by an explicit producer_.Push to avoid losing the message
  // on overflow.
  impl::AsyncAction action(impl::ActionType::kStop);
  producer_.Push(std::move(action));

  consuming_task_.Wait();
  consuming_task_ = {};
}

void TpLogger::Flush() const {
  if (GetSinks().empty()) {
    return;
  }

  if (!in_async_mode_) {
    BackendFlush();
    return;
  }

  const auto flush_snapshot = flush_index_.load();
  impl::AsyncAction action(impl::ActionType::kFlush);
  Push(std::move(action));

  // logger flush is a very rare operation, it is fine to busy-loop here
  if (engine::current_task::GetCurrentTaskContextUnchecked()) {
    while (flush_snapshot == flush_index_.load()) {
      engine::Yield();
    }
  } else {
    while (flush_snapshot == flush_index_.load()) {
      std::this_thread::yield();
    }
  }
}

void TpLogger::Log(Level level, std::string_view msg) const {
  if (GetSinks().empty()) {
    return;
  }

  impl::AsyncAction action{level, std::string{msg}};

  if (!in_async_mode_) {
    BackendLog(std::move(action));
    return;
  }

  Push(std::move(action));
}

void TpLogger::SetPattern(std::string pattern) {
  formatter_pattern_ = std::move(pattern);

  for (const auto& sink : GetSinks()) {
    sink->set_pattern(formatter_pattern_);
  }
}

void TpLogger::AddSink(impl::SinkPtr&& sink) {
  UASSERT(sink);
  sink->set_level(spdlog::level::level_enum::trace);  // Always on
  sink->set_pattern(formatter_pattern_);
  sinks_.push_back(std::move(sink));
}

const std::vector<impl::SinkPtr>& TpLogger::GetSinks() const { return sinks_; }

std::string_view TpLogger::GetLoggerName() const noexcept {
  return logger_name_;
}

void TpLogger::ProcessingLoop() {
  auto consumer = queue_->GetConsumer();

  for (;;) {
    impl::AsyncAction incoming_async_action;
    if (!consumer.Pop(incoming_async_action)) {
      // Producer is dead
      return;
    }

    if (!KeepProcessing(std::move(incoming_async_action))) {
      return;
    }
  }
}

bool TpLogger::KeepProcessing(impl::AsyncAction&& action) {
  switch (action.GetType()) {
    case impl::ActionType::kLog:
      BackendLog(std::move(action));
      break;

    case impl::ActionType::kFlush:
      BackendFlush();
      ++flush_index_;
      break;

    case impl::ActionType::kStop:
      return false;

    default:
      UASSERT(false);
  }
  return true;
}

void TpLogger::Push(impl::AsyncAction&& action) const {
  // Do not do blocking push if we are not in a coroutine context
  if (overflow_policy_ == LoggerConfig::QueueOveflowBehavior::kBlock &&
      engine::current_task::GetCurrentTaskContextUnchecked()) {
    producer_.Push(std::move(action));
  } else {
    UASSERT(overflow_policy_ == LoggerConfig::QueueOveflowBehavior::kDiscard ||
            !engine::current_task::GetCurrentTaskContextUnchecked());
    producer_.PushNoblock(std::move(action));
  }
}

void TpLogger::BackendLog(impl::AsyncAction&& action) const {
  spdlog::details::log_msg msg{};
  msg.logger_name = GetLoggerName();
  msg.level = static_cast<spdlog::level::level_enum>(action.GetLevel());
  msg.time = action.GetTime();
  msg.payload = action.GetPayloadView();

  for (const auto& sink : GetSinks()) {
    if (!sink->should_log(msg.level)) {
      // We could get in here because of the LogRaw, or because log level
      // was changed at runtime, or because socket sink is not listened by
      // testsuite right now.
      continue;
    }

    try {
      sink->log(msg);
    } catch (const std::exception& e) {
      UASSERT_MSG(false, "While writing a log message caught an exception: " +
                             std::string(e.what()));
    }
  }

  if (ShouldFlush(action.GetLevel())) {
    BackendFlush();
  }
}

void TpLogger::BackendFlush() const {
  for (const auto& sink : GetSinks()) {
    try {
      sink->flush();
    } catch (const std::exception& e) {
      UASSERT_MSG(false, "While flushing a log message caught an exception: " +
                             std::string(e.what()));
    }
  }
}

}  // namespace logging::impl

USERVER_NAMESPACE_END
