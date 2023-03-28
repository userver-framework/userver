#include "tp_logger.hpp"

// this header must be included before any spdlog headers
// to override spdlog's level names
#include <logging/spdlog.hpp>

#include <memory>
#include <string>

#include <userver/concurrent/mpsc_queue.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/overloaded.hpp>

#include <engine/task/task_context.hpp>
#include <logging/spdlog_helpers.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

struct TpLogger::ActionVisitor final {
  TpLogger& logger;

  bool operator()(async::Log&& log) const {
    logger.BackendLog(std::move(log));
    return true;
  }

  bool operator()(impl::async::Stop&&) const noexcept { return false; }

  template <class Flush>
  bool operator()(Flush&& flush) const {
    logger.BackendFlush();
    flush.promise.set_value();
    return true;
  }
};

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
                          LoggerConfig::QueueOverflowBehavior overflow_policy) {
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

  ++pending_async_ops_;

  // Some loggings could be in progress. To wait for those we send a Stop
  // message by an explicit producer_.Push to avoid losing the message
  // on overflow.

  engine::TaskCancellationBlocker block_cancel;
  const auto success = producer_.Push(impl::async::Stop{});
  UASSERT(success);
  consuming_task_.Wait();
  consuming_task_ = {};
}

void TpLogger::Flush() const {
  if (GetSinks().empty()) {
    return;
  }

  ++pending_async_ops_;
  if (!in_async_mode_) {
    --pending_async_ops_;
    BackendFlush();
    return;
  }

  // Some loggings could be in progress. To wait for those we send a Flush
  // message by an explicit producer_.Push to avoid losing the message
  // on overflow.

  if (engine::current_task::GetCurrentTaskContextUnchecked()) {
    impl::async::FlushCoro action{};
    auto future = action.promise.get_future();

    engine::TaskCancellationBlocker block_cancel;
    const auto success = producer_.Push(std::move(action));
    UASSERT(success);
    const auto result = future.wait();
    UASSERT(result == engine::FutureStatus::kReady);
  } else {
    impl::async::FlushThreaded action{};
    auto future = action.promise.get_future();

    static constexpr std::size_t kMaxRepetitions = 5;
    for (std::size_t repetitions = kMaxRepetitions; repetitions;
         --repetitions) {
      const auto success = producer_.PushNoblock(std::move(action));
      if (success) {
        future.wait();
        break;
      }

      std::this_thread::yield();
    }
  }
}

void TpLogger::Log(Level level, std::string_view msg) const {
  if (GetSinks().empty()) {
    return;
  }

  impl::async::Log action{level, std::string{msg}};

  ++pending_async_ops_;
  if (!in_async_mode_) {
    --pending_async_ops_;
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

  // touching a local variable instead of a hot `pending_async_ops_` atomic
  std::size_t processed = 0;

  impl::async::Action incoming_async_action;
  for (;;) {
    if (!consumer.Pop(incoming_async_action)) {
      // Producer is dead
      break;
    }

    ++processed;
    if (!KeepProcessing(std::move(incoming_async_action))) {
      break;
    }
  }

  // Process remaining messages from the queue after Stop received

  while (consumer.PopNoblock(incoming_async_action)) {
    ++processed;
    KeepProcessing(std::move(incoming_async_action));
  }

  pending_async_ops_ -= processed;
  while (pending_async_ops_) {
    engine::Yield();
    while (consumer.PopNoblock(incoming_async_action)) {
      --pending_async_ops_;
      KeepProcessing(std::move(incoming_async_action));
    }
  }
}

bool TpLogger::KeepProcessing(impl::async::Action&& action) noexcept {
  try {
    return std::visit(ActionVisitor{*this}, std::move(action));
  } catch (const std::exception& e) {
    UASSERT_MSG(false, fmt::format("Exception while doing an async logging: {}",
                                   e.what()));
  }

  // ActionVisitor::operator() for Stop is noexcept, other operators return true
  return true;
}

void TpLogger::Push(impl::async::Log&& action) const {
  bool success = false;

  // Do not do blocking push if we are not in a coroutine context
  if (overflow_policy_ == LoggerConfig::QueueOverflowBehavior::kBlock &&
      engine::current_task::GetCurrentTaskContextUnchecked()) {
    engine::TaskCancellationBlocker block_cancel;
    success = producer_.Push(std::move(action));
  } else {
    UASSERT(overflow_policy_ == LoggerConfig::QueueOverflowBehavior::kDiscard ||
            !engine::current_task::GetCurrentTaskContextUnchecked());

    success = producer_.PushNoblock(std::move(action));
  }

  if (!success) {
    --pending_async_ops_;
  }
}

void TpLogger::BackendLog(impl::async::Log&& action) const {
  spdlog::details::log_msg msg{};
  msg.logger_name = GetLoggerName();
  msg.level = static_cast<spdlog::level::level_enum>(action.level);
  msg.time = action.time;
  msg.payload = action.payload;

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

  if (ShouldFlush(action.level)) {
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
