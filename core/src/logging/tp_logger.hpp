#pragma once

#include <atomic>
#include <future>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include <userver/engine/condition_variable.hpp>
#include <userver/engine/future.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/logging/format.hpp>
#include <userver/logging/impl/logger_base.hpp>

#include <engine/impl/async_flat_combining_queue.hpp>
#include <logging/config.hpp>
#include <logging/impl/base_sink.hpp>
#include <logging/impl/reopen_mode.hpp>
#include <userver/concurrent/impl/interference_shield.hpp>
#include <userver/concurrent/impl/intrusive_hooks.hpp>
#include <userver/logging/impl/log_stats.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

using SinkPtr = std::unique_ptr<BaseSink>;

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

struct ReopenCoro {
    ReopenMode reopen_mode;
    engine::Promise<void> promise;
};

struct Stop {};

using Action = std::variant<Stop, Log, FlushCoro, FlushThreaded, ReopenCoro>;

struct ActionNode final : public concurrent::impl::SinglyLinkedBaseHook {
    Action action{Stop{}};
};

}  // namespace async

/// @brief Asynchronous logger that logs into a specific TaskProcessor.
class TpLogger final : public LoggerBase {
public:
    TpLogger(Format format, std::string logger_name);
    ~TpLogger() override;

    void StartConsumerTask(
        engine::TaskProcessor& task_processor,
        std::size_t max_queue_size,
        QueueOverflowBehavior overflow_policy
    );

    void StopConsumerTask();

    void Log(Level level, std::string_view msg) override;
    void Flush() override;
    void PrependCommonTags(TagWriter writer) const override;

    void AddSink(impl::SinkPtr&& sink);
    const std::vector<impl::SinkPtr>& GetSinks() const;
    void Reopen(ReopenMode reopen_mode);

    std::string_view GetLoggerName() const noexcept;

    impl::LogStatistics& GetStatistics() noexcept;

protected:
    bool DoShouldLog(Level level) const noexcept override;

private:
    struct ActionVisitor;

    enum class State {
        kSync,
        kAsync,
        kStoppingAsync,
    };

    using Queue = engine::impl::AsyncFlatCombiningQueue;
    using QueueSize = std::int64_t;

    void ProcessingLoop();
    bool HasFreeQueueCapacity() noexcept;
    bool TryWaitFreeQueueCapacity();
    void Push(impl::async::Action&& action);
    void DoPush(concurrent::impl::SinglyLinkedBaseHook& node) noexcept;
    void ConsumeNode(concurrent::impl::SinglyLinkedBaseHook& node) noexcept;
    void ConsumeQueueOnce(Queue::Consumer& consumer) noexcept;
    void CleanUpQueue(Queue::Consumer&& consumer) noexcept;
    void AccountLogConsumed() noexcept;
    void BackendPerform(impl::async::Action&& action) noexcept;
    void BackendLog(impl::async::Log&& action) const;
    void BackendFlush() const;
    void BackendReopen(ReopenMode reopen_mode) const;

    const std::string logger_name_;
    std::vector<impl::SinkPtr> sinks_;
    mutable impl::LogStatistics stats_{};

    engine::Mutex capacity_waiters_mutex_;
    engine::ConditionVariable capacity_waiters_cv_;
    engine::Task consuming_task_;
    std::atomic<QueueSize> max_queue_size_{std::numeric_limits<QueueSize>::max()};
    std::atomic<QueueOverflowBehavior> overflow_policy_{QueueOverflowBehavior::kDiscard};
    // State changes rarely, no need for an InterferenceShield.
    std::atomic<State> state_{State::kSync};
    Queue::Consumer queue_consumer_;
    // A dummy action used for notifying the async task during stopping.
    impl::async::ActionNode stop_node_;

    Queue queue_;
    concurrent::impl::InterferenceShield<std::atomic<QueueSize>> produced_{0};
    concurrent::impl::InterferenceShield<std::atomic<QueueSize>> consumed_{0};
};

}  // namespace logging::impl

USERVER_NAMESPACE_END
