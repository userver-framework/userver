#include <userver/engine/sleep.hpp>

#include <cstdint>
#include <utility>

#include <userver/engine/impl/awaiter.hpp>
#include <userver/engine/impl/context_accessor.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/utils/assert.hpp>

#include <engine/task/task_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

namespace impl {
namespace {
class UnreachableAwaitable final : public AwaitableBase {
public:
    UnreachableAwaitable() = default;

    bool IsReady() const noexcept override { return false; }

    void TryAppendAwaiter(impl::AwaiterPtr& awaiter, std::uintptr_t context) override {
        awaiter_ = std::move(awaiter);
        UASSERT(std::exchange(context_, context) == 0);
    }

    impl::AwaiterPtr RemoveAwaiter(impl::Awaiter& awaiter, std::uintptr_t context) noexcept override {
        UASSERT(awaiter_.get() == &awaiter);
        UASSERT(std::exchange(context_, 0) == context);
        return std::move(awaiter_);
    }

private:
    impl::AwaiterPtr awaiter_;
    std::uintptr_t context_{0};
};

class ImmediateAwaitable final : public AwaitableBase {
public:
    bool IsReady() const noexcept override { return notified_; }

    void TryAppendAwaiter(AwaiterPtr& awaiter, std::uintptr_t context) override {
        notified_ = true;
        NotifyAndDispose(std::move(awaiter), context);
    }

    AwaiterPtr RemoveAwaiter(Awaiter&, std::uintptr_t) noexcept override { return {}; }

private:
    bool notified_{false};
};

class TaskBackgroundScope final {
public:
    explicit TaskBackgroundScope(TaskContext& context)
        : context_(context),
          previous_background_flag_(context_.IsBackground())
    {
        context_.SetBackground(true);
    }

    ~TaskBackgroundScope() { context_.SetBackground(previous_background_flag_); }

    TaskBackgroundScope(const TaskBackgroundScope&) = delete;
    TaskBackgroundScope(TaskBackgroundScope&&) = delete;
    TaskBackgroundScope& operator=(const TaskBackgroundScope&) = delete;
    TaskBackgroundScope& operator=(TaskBackgroundScope&&) = delete;

private:
    TaskContext& context_;
    const bool previous_background_flag_;
};

}  // namespace
}  // namespace impl

void InterruptibleSleepUntil(Deadline deadline) {
    auto& current = current_task::GetCurrentTaskContext();
    const impl::TaskBackgroundScope background_scope{current};
    impl::UnreachableAwaitable awaitable{};
    current.Sleep(awaitable, deadline);
}

void SleepUntil(Deadline deadline) {
    const TaskCancellationBlocker block_cancel;
    InterruptibleSleepUntil(deadline);
}

void Yield() {
    const TaskCancellationBlocker block_cancel;
    auto& current = current_task::GetCurrentTaskContext();
    const impl::TaskBackgroundScope background_scope{current};
    impl::ImmediateAwaitable awaitable;
    current.Sleep(awaitable, Deadline{});
}

}  // namespace engine

USERVER_NAMESPACE_END
