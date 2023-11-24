#include "tp_logger.hpp"

#include <fmt/format.h>

#include <engine/task/task_context.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/logging/impl/tag_writer.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/enumerate.hpp>
#include <userver/utils/fast_scope_guard.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

struct TpLogger::ActionVisitor final {
  TpLogger& logger;

  void operator()(impl::async::Log&& log) const {
    logger.AccountLogConsumed();
    logger.BackendLog(std::move(log));
  }

  void operator()(impl::async::Stop&&) const noexcept {
    // The consumer thread will check state_ later.
  }

  void operator()(impl::async::ReopenCoro&& reopen) const noexcept {
    try {
      logger.BackendReopen(reopen.reopen_mode);
      reopen.promise.set_value();
    } catch (const std::exception& e) {
      // For exceptions not inherited from std::exception, a broken promise will
      // be thrown out of the future.
      reopen.promise.set_exception(std::make_exception_ptr(e));
    }
  }

  template <class Flush>
  void operator()(Flush&& flush) const {
    logger.BackendFlush();
    flush.promise.set_value();
  }
};

TpLogger::TpLogger(Format format, std::string logger_name)
    : LoggerBase(format), logger_name_(std::move(logger_name)) {
  SetLevel(logging::Level::kInfo);
}

void TpLogger::StartConsumerTask(engine::TaskProcessor& task_processor,
                                 std::size_t max_queue_size,
                                 QueueOverflowBehavior overflow_policy) {
  UINVARIANT(max_queue_size != 0 && max_queue_size <= (std::size_t{1} << 31),
             "Invalid max queue size");
  max_queue_size_.store(max_queue_size);
  overflow_policy_.store(overflow_policy);

  auto expected = State::kSync;
  const bool success = state_.compare_exchange_strong(expected, State::kAsync);
  UINVARIANT(success, "Logger can only be switched to async mode once");

  {
    // Lock the consumer synchronously.
    const engine::TaskCancellationBlocker block_cancel;
    queue_consumer_ = queue_.WaitAndStartConsuming();
  }

  // Make sure to stop consuming even if Async throws.
  utils::FastScopeGuard exit_async_guard([this]() noexcept {
    state_.store(State::kSync);
    queue_consumer_ = {};
  });

  consuming_task_ = engine::CriticalAsyncNoSpan(
      task_processor,
      [this, guard = std::move(exit_async_guard)] { ProcessingLoop(); });
}

TpLogger::~TpLogger() {
  UASSERT_MSG(
      state_ == State::kSync,
      "We may be in non coroutine context, async logger must be in sync mode");
  UASSERT_MSG(!consuming_task_.IsValid(),
              "We may be in non coroutine context, async logger must be in "
              "sync mode and consuming task must be stopped");
}

void TpLogger::StopConsumerTask() {
  auto expected = State::kAsync;
  if (!state_.compare_exchange_strong(expected, State::kStoppingAsync)) {
    return;
  }

  DoPush(stop_node_);

  const engine::TaskCancellationBlocker block_cancel;
  consuming_task_.Wait();
  consuming_task_ = {};
}

void TpLogger::Flush() {
  if (GetSinks().empty()) {
    return;
  }

  if (engine::current_task::IsTaskProcessorThread()) {
    impl::async::FlushCoro action{};
    auto future = action.promise.get_future();

    Push(std::move(action));

    const engine::TaskCancellationBlocker block_cancel;
    future.get();
  } else {
    impl::async::FlushThreaded action{};
    auto future = action.promise.get_future();

    Push(std::move(action));

    future.get();
  }
}

statistics::LogStatistics& TpLogger::GetStatistics() noexcept { return stats_; }

void TpLogger::Log(Level level, std::string_view msg) {
  ++stats_.by_level[static_cast<std::size_t>(level)];

  if (GetSinks().empty()) {
    return;
  }

  impl::async::Log action{level, std::string{msg}};

  if (TryWaitFreeQueueCapacity()) {
    // The queue might have concurrently become full, in which case the size
    // will temporarily go over the max size. The actual number of log actions
    // in queue_ will not typically go over max_size + n_threads.
    produced_->fetch_add(1);

    Push(std::move(action));
  } else {
    ++stats_.dropped;
  }
}

void TpLogger::PrependCommonTags(TagWriter writer) const {
  auto* const span = tracing::Span::CurrentSpanUnchecked();
  if (span) span->LogTo(writer);

  auto* const task = engine::current_task::GetCurrentTaskContextUnchecked();
  writer.PutTag("task_id", HexShort{task});

  auto* const thread_id = reinterpret_cast<void*>(pthread_self());
  writer.PutTag("thread_id", Hex{thread_id});
}

bool TpLogger::DoShouldLog(Level level) const noexcept {
  const auto* const span = tracing::Span::CurrentSpanUnchecked();
  if (span) {
    const auto local_log_level = span->GetLocalLogLevel();
    if (local_log_level && *local_log_level > level) {
      return false;
    }
  }

  return true;
}

void TpLogger::AddSink(impl::SinkPtr&& sink) {
  UASSERT(sink);
  sinks_.push_back(std::move(sink));
}

const std::vector<impl::SinkPtr>& TpLogger::GetSinks() const { return sinks_; }

void TpLogger::Reopen(ReopenMode reopen_mode) {
  if (GetSinks().empty()) {
    return;
  }

  UASSERT(engine::current_task::IsTaskProcessorThread());
  impl::async::ReopenCoro action{reopen_mode, {}};
  auto future = action.promise.get_future();

  Push(std::move(action));

  const engine::TaskCancellationBlocker block_cancel;
  future.get();
}

std::string_view TpLogger::GetLoggerName() const noexcept {
  return logger_name_;
}

void TpLogger::ProcessingLoop() {
  const engine::TaskCancellationBlocker cancel_blocker;

  while (true) {
    ConsumeQueueOnce(queue_consumer_);
    if (state_ != State::kAsync) {
      UASSERT(state_ == State::kStoppingAsync);
      break;
    }
    queue_.WaitWhileEmpty(queue_consumer_);
  }

  CleanUpQueue(std::move(queue_consumer_));
}

void TpLogger::BackendPerform(impl::async::Action&& action) noexcept {
  try {
    std::visit(ActionVisitor{*this}, std::move(action));
  } catch (const std::exception& e) {
    UASSERT_MSG(false, fmt::format("Exception while doing an async logging: {}",
                                   e.what()));
  }
}

bool TpLogger::HasFreeQueueCapacity() noexcept {
  return produced_->load() - consumed_->load() < max_queue_size_.load();
}

bool TpLogger::TryWaitFreeQueueCapacity() {
  if (HasFreeQueueCapacity()) {
    return true;
  }

  // Do not do blocking push if we are not in a coroutine context.
  if (overflow_policy_.load() != QueueOverflowBehavior::kBlock ||
      !engine::current_task::IsTaskProcessorThread()) {
    return false;
  }

  const engine::TaskCancellationBlocker block_cancel;
  std::unique_lock lock{capacity_waiters_mutex_};
  [[maybe_unused]] const bool success = capacity_waiters_cv_.Wait(
      lock, [this] { return HasFreeQueueCapacity(); });
  UASSERT(success);
  return true;
}

void TpLogger::Push(impl::async::Action&& action) {
  auto node = std::make_unique<impl::async::ActionNode>();
  node->action = std::move(action);
  DoPush(*node.release());
}

void TpLogger::DoPush(concurrent::impl::SinglyLinkedBaseHook& node) noexcept {
  auto consumer = queue_.PushAndTryStartConsuming(node);
  if (consumer.IsValid()) {
    CleanUpQueue(std::move(consumer));
  }
}

void TpLogger::AccountLogConsumed() noexcept {
  consumed_->store(consumed_->load(std::memory_order_relaxed) + 1,
                   std::memory_order_relaxed);
  if (overflow_policy_.load() == QueueOverflowBehavior::kBlock) {
    {
      // Atomic consumed_ mutation doesn't need to be protected by lock.
      // With this lock in place, a waiter can check + wait either:
      // 1. before us locking, then we will notify the waiter, or
      // 2. after us locking, then the waiter will receive our updates and
      //    not fall asleep
      const std::lock_guard lock{capacity_waiters_mutex_};
    }
    capacity_waiters_cv_.NotifyOne();
  }
}

void TpLogger::ConsumeNode(
    concurrent::impl::SinglyLinkedBaseHook& node) noexcept {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
  auto& action_node = static_cast<impl::async::ActionNode&>(node);
  if (&action_node == &stop_node_) return;

  BackendPerform(std::move(action_node.action));
  delete &action_node;
}

void TpLogger::ConsumeQueueOnce(Queue::Consumer& consumer) noexcept {
  while (auto* const node_base = consumer.TryPop()) {
    ConsumeNode(*node_base);
  }
}

void TpLogger::CleanUpQueue(Queue::Consumer&& consumer) noexcept {
  std::move(consumer).ConsumeAndStop(
      [this](auto& node) noexcept { ConsumeNode(node); });
}

void TpLogger::BackendLog(impl::async::Log&& action) const {
  LogMessage message;
  message.payload = action.payload;
  message.level = action.level;

  for (const auto& sink : GetSinks()) {
    try {
      sink->Log(message);
    } catch (const std::exception& e) {
      UASSERT_MSG(false, "While writing a log message caught an exception: " +
                             std::string(e.what()));
    }
  }

  if (ShouldFlush(message.level)) {
    BackendFlush();
  }
}

void TpLogger::BackendFlush() const {
  for (const auto& sink : GetSinks()) {
    try {
      sink->Flush();
    } catch (const std::exception& e) {
      UASSERT_MSG(false, "While flushing a log message caught an exception: " +
                             std::string(e.what()));
    }
  }
}

void TpLogger::BackendReopen(ReopenMode reopen_mode) const {
  std::string result_messages{};
  for (const auto& [index, sink] : utils::enumerate(GetSinks())) {
    try {
      sink->Reopen(reopen_mode);
    } catch (const std::exception& e) {
      result_messages += e.what();
      result_messages += "; ";
      LOG_ERROR() << "Exception on log reopen in sink #" << index
                  << " of logger '" << GetLoggerName() << "': " << e;
    }
  }
  if (!result_messages.empty()) {
    stats_.has_reopening_error.store(true);
    throw std::runtime_error("BackendReopen errors: " + result_messages);
  }
  stats_.has_reopening_error.store(false);
}

}  // namespace logging::impl

USERVER_NAMESPACE_END
