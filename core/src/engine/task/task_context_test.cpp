#include <gtest/gtest.h>

#include <engine/task/sleep_state.hpp>
#include <engine/task/task_context.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/sleep.hpp>

#include <userver/engine/task/task_base.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

// TAXICOMMON-347 -- Task closure gets destroyed in TaskProcessor::ProcessTasks.
//
// The task was detached (no coroutine ownership) and it got cancelled as
// the processor shut down. As the task hasn't started yet, we invoked the fast
// path that did not acquire a coroutine, and payload was destroyed in
// an unexpected context.
namespace {
class DtorInCoroChecker final {
public:
    ~DtorInCoroChecker() { EXPECT_TRUE(engine::current_task::IsTaskProcessorThread()); }
};

// TAXICOMMON-4406 -- Wait list notification before cleanup gets lost
//
// The task had been woken up by a deadline timer and got notified by a wait
// list in the process of housekeeping. This notification was not accounted as a
// wakeup source because wakeup source was calculated early.
struct WaitListRaceSimulator final : public engine::impl::WeakAwaitable {
    // cannot use passed deadline because of fast path
    WaitListRaceSimulator() = default;

    bool IsReady() const noexcept override { return false; }

    void TryAppendAwaiter(engine::impl::AwaiterPtr& awaiter, std::uintptr_t context) override {
        awaiter_ = std::move(awaiter);
        context_ = context;

        // wake up immediately
        auto& current = engine::current_task::GetCurrentTaskContext();
        UASSERT(awaiter_.get() == static_cast<engine::impl::Awaiter*>(&current));
        engine::impl::TaskContext::Wakeup(
            boost::intrusive_ptr<engine::impl::TaskContext>{&current},
            engine::impl::TaskContext::WakeupSource::kDeadlineTimer,
            static_cast<engine::impl::Epoch>(context)
        );
    }

    engine::impl::AwaiterPtr RemoveAwaiter(engine::impl::Awaiter& awaiter, std::uintptr_t context) noexcept override {
        UASSERT(awaiter_.get() == &awaiter);
        UASSERT(context_ == context);

        // simulate wait list notification before cleanup
        engine::impl::NotifyAndDispose(std::move(awaiter_), context_);
        return nullptr;
    }

private:
    engine::impl::AwaiterPtr awaiter_;
    std::uintptr_t context_{0};
};

constexpr size_t kWorkerThreads = 1;

}  // namespace

UTEST_MT(TaskContext, DetachedAndCancelledOnStart, kWorkerThreads) {
    auto task = engine::AsyncNoTracing([checker = DtorInCoroChecker()] { FAIL() << "Cancelled task started"; });
    task.RequestCancel();
    engine::DetachUnscopedUnsafe(std::move(task));
}

UTEST_MT(TaskContext, DetachedAndCancelledOnStartWithWrappedCall, kWorkerThreads) {
    auto task = engine::AsyncNoTracing([](auto&&) { FAIL() << "Cancelled task started"; }, DtorInCoroChecker());
    task.RequestCancel();
    engine::DetachUnscopedUnsafe(std::move(task));
}

UTEST(TaskContext, WaitInterruptedReason) {
    auto long_task = engine::AsyncNoTracing([] { engine::InterruptibleSleepFor(std::chrono::seconds{5}); });
    auto waiter = engine::AsyncNoTracing([&] {
        auto reason = engine::TaskCancellationReason::kNone;
        try {
            long_task.Get();
        } catch (const engine::WaitInterruptedException& ex) {
            reason = ex.Reason();
        }
        EXPECT_EQ(engine::TaskCancellationReason::kUserRequest, reason);
    });

    engine::Yield();
    ASSERT_EQ(engine::Task::State::kSuspended, waiter.GetState());
    waiter.RequestCancel();
    waiter.Get();
}

UTEST(TaskContext, WaitListWakeupRace) {
    auto& context = engine::current_task::GetCurrentTaskContext();

    WaitListRaceSimulator awaitable;
    EXPECT_EQ(context.Sleep(awaitable, engine::Deadline{}), engine::impl::TaskContext::WakeupSource::kNotify);
}

USERVER_NAMESPACE_END
