#include <userver/engine/sleep.hpp>

#include <array>
#include <cstddef>
#include <vector>

#include <userver/engine/async.hpp>
#include <userver/engine/impl/context_accessor.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/engine/task/current_task.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/fast_scope_guard.hpp>

#include <engine/plugin.hpp>
#include <engine/task/task_context.hpp>
#include <engine/task/task_processor.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr std::size_t kYieldCount = 8;
constexpr std::size_t kSingleWorkerCount = 1;
constexpr std::array kBackgroundStates{false, true};

class YieldObserver final : public engine::PluginBase {
public:
    void Observe(const engine::impl::TaskContext& task) noexcept { task_id_ = task.GetTaskId(); }

    void HookBeforeSleep(engine::impl::TaskContext& task) noexcept override {
        if (task.GetTaskId() != task_id_) {
            return;
        }
        ++sleep_count_;
        EXPECT_TRUE(task.IsBackground());
        EXPECT_FALSE(task.IsCancellable());
    }

    void HookAfterWakeup(engine::impl::TaskContext& task) noexcept override {
        if (task.GetTaskId() == task_id_) {
            ++wakeup_count_;
        }
    }

    std::size_t GetSleepCount() const noexcept { return sleep_count_; }
    std::size_t GetWakeupCount() const noexcept { return wakeup_count_; }

private:
    engine::impl::TaskContext::TaskId task_id_{0};
    std::size_t sleep_count_{0};
    std::size_t wakeup_count_{0};
};

enum class Step { kChildStarted, kParentResumed, kChildResumed };

}  // namespace

UTEST(EngineYield, DoesNotStartDeadlineTimer) {
    auto task = engine::AsyncNoTracing([] {
        const auto& context = engine::current_task::GetCurrentTaskContext();
        ASSERT_FALSE(context.GetCancelDeadline().IsReachable());
        const auto references_before_yield = context.UseCount();
        for (std::size_t index = 0; index < kYieldCount; ++index) {
            engine::Yield();
            EXPECT_EQ(context.UseCount(), references_before_yield);
        }
    });
    task.Get();
}

UTEST_MT(EngineYield, LetsReadyTasksRun, kSingleWorkerCount) {
    std::vector<Step> steps;
    auto task = engine::AsyncNoTracing([&steps] {
        steps.push_back(Step::kChildStarted);
        engine::Yield();
        steps.push_back(Step::kChildResumed);
    });
    EXPECT_TRUE(steps.empty());
    engine::Yield();
    steps.push_back(Step::kParentResumed);
    task.Get();
    const std::vector expected{Step::kChildStarted, Step::kParentResumed, Step::kChildResumed};
    EXPECT_EQ(steps, expected);
}

UTEST_MT(EngineYield, PreservesBackgroundAndCancellationState, kSingleWorkerCount) {
    auto& processor = engine::current_task::GetTaskProcessor();
    YieldObserver observer;
    processor.RegisterPlugin(observer);
    const utils::FastScopeGuard unregister{[&]() noexcept { processor.UnregisterPlugin(observer); }};
    auto task = engine::AsyncNoTracing([&observer] {
        auto& context = engine::current_task::GetCurrentTaskContext();
        observer.Observe(context);
        for (const bool background : kBackgroundStates) {
            context.SetBackground(background);
            const bool cancellable = context.IsCancellable();
            engine::Yield();
            EXPECT_EQ(context.IsBackground(), background);
            EXPECT_EQ(context.IsCancellable(), cancellable);
        }
    });
    task.Get();
    EXPECT_EQ(observer.GetSleepCount(), kBackgroundStates.size());
    EXPECT_EQ(observer.GetWakeupCount(), kBackgroundStates.size());
}

UTEST_MT(EngineYield, PendingCancellationStillAllowsOtherTasksToRun, kSingleWorkerCount) {
    bool ready_task_ran = false;
    auto ready_task = engine::AsyncNoTracing([&] { ready_task_ran = true; });
    engine::current_task::RequestCancel();
    EXPECT_TRUE(engine::current_task::ShouldCancel());
    engine::Yield();
    EXPECT_TRUE(ready_task_ran);
    EXPECT_TRUE(engine::current_task::ShouldCancel());
    const engine::TaskCancellationBlocker cancellation_blocker;
    ready_task.Get();
}

USERVER_NAMESPACE_END
