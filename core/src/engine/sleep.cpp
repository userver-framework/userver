#include <userver/engine/sleep.hpp>

#include <utility>

#include <userver/engine/task/cancel.hpp>

#include <engine/task/task_context.hpp>
#include <userver/utils/fast_scope_guard.hpp>

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
}  // namespace
}  // namespace impl

void InterruptibleSleepUntil(Deadline deadline) {
    auto& current = current_task::GetCurrentTaskContext();
    const utils::FastScopeGuard
        reset_background([&current, previous_background_flag = current.IsBackground()]() noexcept {
            current.SetBackground(previous_background_flag);
        });
    current.SetBackground(true);
    impl::UnreachableAwaitable awaitable{};
    current.Sleep(awaitable, deadline);
}

void SleepUntil(Deadline deadline) {
    const TaskCancellationBlocker block_cancel;
    InterruptibleSleepUntil(deadline);
}

void Yield() { SleepUntil(Deadline::Passed()); }

}  // namespace engine

USERVER_NAMESPACE_END
