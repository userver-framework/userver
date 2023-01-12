#include "tp_logger.hpp"

#include <memory>
#include <string>

#include <userver/concurrent/mpsc_queue.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/logging/log.hpp>

#include <engine/task/task_context.hpp>
#include <logging/logger_with_info.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {

TpLogger::TpLogger(std::string logger_name,
                   engine::TaskProcessor& task_processor,
                   std::size_t max_queue_size,
                   LoggerConfig::QueueOveflowBehavior overflow_policy)
    : logger(std::move(logger_name)),
      queue_(Queue::Create(max_queue_size)),
      consuming_task_(engine::CriticalAsyncNoSpan(
          task_processor, &TpLogger::ProcessingLoop, this)),
      producer_(queue_->GetMultiProducer()),
      overflow_policy_(overflow_policy) {}

TpLogger::~TpLogger() {
  UASSERT_MSG(
      in_fallback_mode_,
      "We may be in non coroutine context, async logger must be in sync mode");
  UASSERT_MSG(!consuming_task_.IsValid(),
              "We may be in non coroutine context, async logger must be in "
              "sync mode and consuming task must be stopped");
}

void TpLogger::SwitchToSyncMode() {
  const auto was_fallback = in_fallback_mode_.exchange(true);
  UASSERT(!was_fallback);

  // Some loggings could be in progress. To wait for those we send a kStop
  // message by an explicit producer_.Push to avoid losing the message
  // on overflow.
  impl::AsyncAction action(impl::ActionType::kStop);
  producer_.Push(std::move(action));

  consuming_task_.Wait();
  consuming_task_ = {};
}

std::shared_ptr<spdlog::logger> TpLogger::clone(std::string /*new_name*/) {
  UINVARIANT(false, "Not implemented");
}

void TpLogger::sink_it_(const spdlog::details::log_msg& msg) {
  if (sinks_.empty()) {
    return;
  }

  if (in_fallback_mode_) {
    BackendSinkIt(msg);
    return;
  }

  impl::AsyncAction action(impl::ActionType::kLog, msg);
  Push(std::move(action));
}

void TpLogger::flush_() {
  if (sinks_.empty()) {
    return;
  }

  if (in_fallback_mode_) {
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

void TpLogger::ProcessingLoop() {
  auto consumer = queue_->GetConsumer();

  for (;;) {
    impl::AsyncAction incoming_async_action;
    if (!consumer.Pop(incoming_async_action)) {
      // Producer is dead
      return;
    }

    if (!KeepProcessing(incoming_async_action)) {
      return;
    }
  }
}

bool TpLogger::KeepProcessing(const impl::AsyncAction& action) {
  switch (action.GetType()) {
    case impl::ActionType::kLog:
      BackendSinkIt(action);
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

void TpLogger::Push(impl::AsyncAction&& action) {
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

void TpLogger::BackendSinkIt(const spdlog::details::log_msg& msg) {
  for (auto& sink : sinks_) {
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

  if (should_flush_(msg)) {
    BackendFlush();
  }
}

void TpLogger::BackendFlush() {
  for (auto& sink : sinks_) {
    try {
      sink->flush();
    } catch (const std::exception& e) {
      UASSERT_MSG(false, "While flushing a log message caught an exception: " +
                             std::string(e.what()));
    }
  }
}

}  // namespace logging

USERVER_NAMESPACE_END
