#include "tp_logger.hpp"

#include <algorithm>

#include <fmt/format.h>
#include <boost/container/static_vector.hpp>

#include <engine/task/task_context.hpp>
#include <userver/concurrent/impl/intrusive_thread_unsafe_slist.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/logging/impl/tag_writer.hpp>
#include <userver/logging/log.hpp>
#include <userver/logging/logger.hpp>
#include <userver/utils/enumerate.hpp>
#include <userver/utils/fast_scope_guard.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

namespace {

template <class It>
inline std::size_t AdvanceOverLogs(It& it, It end, std::size_t length) noexcept {
    std::size_t advance_count = 0;
    for (; it != end && advance_count != length; ++it, ++advance_count) {
        if (!std::holds_alternative<impl::async::Log>(it->action)) {
            break;
        }
    }

    return advance_count;
}

using IoVec = struct iovec;
constexpr std::size_t kMaxVectorBatchSize = 1024;  // Max known to me IOV_MAX
using PendingLogMessages = boost::container::static_vector<IoVec, kMaxVectorBatchSize>;

template <class It>
void CollectPendingLogMessages(It begin, It end, Level sink_level, PendingLogMessages& pending_log_messages) noexcept {
    for (auto it = begin; it != end; ++it) {
        auto* log_message = std::get_if<async::Log>(&it->action);
        UASSERT(log_message);

        if (log_message->level < sink_level) {
            continue;
        }

        pending_log_messages.push_back(IoVec{
            .iov_base = log_message->payload.data(),  // non const, but does not change the pointed-to value
            .iov_len = log_message->payload.size(),
        });
    }
}

}  // namespace

struct TpLogger::ActionVisitor final {
    TpLogger& logger;

    void operator()(impl::async::Log&&) const noexcept {
        UASSERT_MSG(false, "ConsumeQueueOnce must deal with all the impl::async::Log");
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
        flush.promise.set_value();
    }
};

TpLogger::TpLogger(Format format, std::string logger_name)
    : impl::TextLogger(format),
      logger_name_(std::move(logger_name))
{
    SetLevel(logging::Level::kInfo);
}

void TpLogger::StartConsumerTask(
    engine::TaskProcessor& task_processor,
    std::size_t max_queue_size,
    QueueOverflowBehavior overflow_policy,
    std::size_t flush_queue_size
) {
    UINVARIANT(max_queue_size != 0 && max_queue_size <= (std::size_t{1} << 31), "Invalid max queue size");
    max_queue_size_.store(max_queue_size);
    flush_queue_size_.store(std::min(flush_queue_size, max_queue_size / 2));
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

    consuming_task_ = engine::CriticalAsyncNoTracing(task_processor, [this, guard = std::move(exit_async_guard)] {
        ProcessingLoop();
    });
}

TpLogger::~TpLogger() {
    UASSERT_MSG(state_ == State::kSync, "We may be in non coroutine context, async logger must be in sync mode");
    UASSERT_MSG(
        !consuming_task_.IsValid(),
        "We may be in non coroutine context, async logger must be in "
        "sync mode and consuming task must be stopped"
    );
}

void TpLogger::StopConsumerTask() {
    auto expected = State::kAsync;
    if (!state_.compare_exchange_strong(expected, State::kStoppingAsync)) {
        return;
    }

    DoPush(stop_node_, Queue::NotificationMode::kNotify);

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

        Push(std::move(action), Queue::NotificationMode::kNotify);

        const engine::TaskCancellationBlocker block_cancel;
        future.get();
    } else {
        impl::async::FlushThreaded action{};
        auto future = action.promise.get_future();

        Push(std::move(action), Queue::NotificationMode::kNotify);

        future.get();
    }
}

impl::LogStatistics& TpLogger::GetStatistics() noexcept { return stats_; }

void TpLogger::Log(Level level, impl::formatters::LoggerItemRef item) {
    UASSERT(dynamic_cast<impl::TextLogItem*>(&item));
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
    auto& msg = static_cast<impl::TextLogItem&>(item);
    ++stats_.by_level[static_cast<std::size_t>(level)];

    if (GetSinks().empty()) {
        return;
    }

    if (TryWaitFreeQueueCapacity()) {
        // The queue might have concurrently become full, in which case the size
        // will temporarily go over the max size. The actual number of log actions
        // in queue_ will not typically go over max_size + n_threads.
        const auto produced = produced_->fetch_add(1) + 1;

        // Wake the consumer for logs that must be flushed immediately (level at or above the flush level),
        // or once the queue reaches the configured flush size;
        // otherwise leave the draining to the periodic flush.
        const bool should_notify =
            ShouldFlush(level) || !notification_batching_.load() ||
            (produced - consumed_->load()) >= flush_queue_size_.load();

        try {
            Push(
                impl::async::Log{level, utils::FixedArray<char>(msg.log_line.begin(), msg.log_line.end())},
                should_notify ? Queue::NotificationMode::kNotify : Queue::NotificationMode::kDeferred
            );
        } catch (const std::exception&) {
            // failed to construct a Log action or a node in Push
            produced_->fetch_sub(1);
            throw;
        }
    } else {
        ++stats_.dropped;
    }
}

void TpLogger::PrependCommonTags(TagWriter writer) const { impl::PrependCommonTags(writer, GetLevel()); }

void TpLogger::AddSink(impl::SinkPtr&& sink) {
    UASSERT(sink);
    sinks_.push_back(std::move(sink));
}

const std::vector<impl::SinkPtr>& TpLogger::GetSinks() const { return sinks_; }

void TpLogger::SetNotificationBatching(bool enabled) noexcept { notification_batching_.store(enabled); }

void TpLogger::Reopen(ReopenMode reopen_mode) {
    if (GetSinks().empty()) {
        return;
    }

    UASSERT(engine::current_task::IsTaskProcessorThread());
    impl::async::ReopenCoro action{reopen_mode, {}};
    auto future = action.promise.get_future();

    Push(std::move(action), Queue::NotificationMode::kNotify);

    const engine::TaskCancellationBlocker block_cancel;
    future.get();
}

std::string_view TpLogger::GetLoggerName() const noexcept { return logger_name_; }

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
        UASSERT_MSG(false, fmt::format("Exception while doing an async logging: {}", e.what()));
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
    if (overflow_policy_.load() != QueueOverflowBehavior::kBlock || !engine::current_task::IsTaskProcessorThread()) {
        return false;
    }

    const engine::TaskCancellationBlocker block_cancel;
    std::unique_lock lock{capacity_waiters_mutex_};
    [[maybe_unused]] const bool success = capacity_waiters_cv_.Wait(lock, [this] { return HasFreeQueueCapacity(); });
    UASSERT(success);
    return true;
}

void TpLogger::Push(impl::async::Action&& action, Queue::NotificationMode notify) {
    auto node = std::make_unique<impl::async::ActionNode>();
    node->action = std::move(action);
    DoPush(*node.release(), notify);
}

void TpLogger::DoPush(concurrent::impl::SinglyLinkedBaseHook& node, Queue::NotificationMode notify) noexcept {
    auto consumer = queue_.PushAndTryStartConsuming(node, notify);
    if (consumer.IsValid()) {
        CleanUpQueue(std::move(consumer));
    }
}

void TpLogger::AccountLogConsumed(std::size_t count) noexcept {
    consumed_->store(consumed_->load(std::memory_order_relaxed) + count, std::memory_order_relaxed);
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

void TpLogger::PopActionNodes(Queue::Consumer& consumer, ActionNodesSlist& nodes_slist) noexcept {
    auto last_node = nodes_slist.begin();
    while (auto* const node_base = consumer.TryPop()) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
        if (static_cast<impl::async::ActionNode*>(node_base) == &stop_node_) {
            break;
        }

        last_node = nodes_slist.Adopt(last_node, node_base);
    }
}

void TpLogger::ConsumeQueueOnce(Queue::Consumer& consumer) noexcept {
    ActionNodesSlist nodes_slist;
    PopActionNodes(consumer, nodes_slist);

    PendingLogMessages pending_log_messages;

    if (nodes_slist.empty()) [[unlikely]] {
        return;
    }

    for (;;) {
        auto chunk_end = nodes_slist.begin();
        const std::size_t distance = AdvanceOverLogs(chunk_end, nodes_slist.end(), kMaxVectorBatchSize);
        AccountLogConsumed(distance);

        const auto& sinks = GetSinks();
        for (auto sink_it = sinks.begin(); sink_it != sinks.end();) {
            const auto sink_level = (*sink_it)->GetLevel();
            if (sink_level == Level::kNone) [[unlikely]] {
                ++sink_it;
                continue;
            }

            CollectPendingLogMessages(nodes_slist.begin(), chunk_end, sink_level, pending_log_messages);

            const std::span messages(pending_log_messages.data(), pending_log_messages.size());
            do {
                if (!messages.empty()) [[likely]] {
                    try {
                        (*sink_it)->Write(messages);
                    } catch (const std::exception& e) {
                        UASSERT_MSG(false, "While writing a log message caught an exception: " + std::string(e.what()));
                    }
                }
                ++sink_it;
            } while (sink_it != sinks.end() && (*sink_it)->GetLevel() == sink_level);

            pending_log_messages.clear();
        }

        while (chunk_end != nodes_slist.end() && !std::holds_alternative<async::Log>(chunk_end->action)) {
            BackendPerform(std::move(chunk_end->action));
            ++chunk_end;
        }

        nodes_slist.EraseFromBegin(chunk_end);

        if (nodes_slist.empty()) {
            break;
        }
    }
}

void TpLogger::CleanUpQueue(Queue::Consumer&& consumer) noexcept {
    do {
        ConsumeQueueOnce(consumer);
    } while (!consumer.TryStopConsuming());
}

void TpLogger::BackendReopen(ReopenMode reopen_mode) const {
    std::string result_messages{};
    for (const auto& [index, sink] : utils::enumerate(GetSinks())) {
        try {
            sink->Reopen(reopen_mode);
        } catch (const std::exception& e) {
            result_messages += e.what();
            result_messages += "; ";
            LOG_ERROR()
                << "Exception on log reopen in sink #" << index << " of logger '" << GetLoggerName() << "': " << e;
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
