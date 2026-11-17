#include <engine/plugin.hpp>

#include <vector>

#include <gmock/gmock.h>

#include <userver/engine/async.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/engine/task/current_task.hpp>
#include <userver/engine/task/local_variable.hpp>
#include <userver/utest/utest.hpp>

#include <engine/task/task_context.hpp>
#include <engine/task/task_processor.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

using TaskState = engine::Task::State;

struct YieldOnDestruction final {
    ~YieldOnDestruction() { engine::Yield(); }
};

engine::TaskLocalVariable<YieldOnDestruction> kYieldOnDestruction;

// Both parent and child call these hooks, so the probe requires a single worker.
class PendingFinalStateProbe final : public engine::PluginBase {
public:
    void HookTaskStart(engine::impl::TaskContext& task) noexcept override {
        task_ = &task;
        kYieldOnDestruction.GetOrEmplace();
    }

    void HookBeforeSleep(engine::impl::TaskContext& task) noexcept override {
        if (&task == task_) {
            sleep_states_.push_back(task.GetPendingFinalState());
        }
    }

    void HookTaskStop(engine::impl::TaskContext& task) noexcept override {
        if (&task == task_) {
            stop_states_.push_back(task.GetPendingFinalState());
        }
    }

    const std::vector<TaskState>& GetSleepPendingFinalStates() const noexcept { return sleep_states_; }

    const std::vector<TaskState>& GetStopPendingFinalStates() const noexcept { return stop_states_; }

private:
    std::vector<TaskState> sleep_states_;
    std::vector<TaskState> stop_states_;
    const engine::impl::TaskContext* task_{nullptr};
};

class EnginePluginPendingFinalState : public ::testing::Test {
protected:
    EnginePluginPendingFinalState() {
        [] { ASSERT_EQ(engine::current_task::GetWorkerCount(), 1); }();
        engine::current_task::GetTaskProcessor().RegisterPlugin(plugin_);
    }

    ~EnginePluginPendingFinalState() override { engine::current_task::GetTaskProcessor().UnregisterPlugin(plugin_); }

    const PendingFinalStateProbe& GetPlugin() const { return plugin_; }

private:
    PendingFinalStateProbe plugin_;
};

}  // namespace

UTEST_F(EnginePluginPendingFinalState, Completed) {
    auto task = engine::AsyncNoTracing([] {
        EXPECT_EQ(engine::current_task::GetCurrentTaskContext().GetPendingFinalState(), TaskState::kInvalid);
        engine::Yield();
    });
    task.Wait();

    EXPECT_THAT(
        GetPlugin().GetSleepPendingFinalStates(),
        ::testing::ElementsAre(TaskState::kInvalid, TaskState::kCompleted)
    );
    EXPECT_THAT(GetPlugin().GetStopPendingFinalStates(), ::testing::ElementsAre(TaskState::kCompleted));
    EXPECT_EQ(task.GetState(), TaskState::kCompleted);
}

UTEST_F(EnginePluginPendingFinalState, CancelledBeforeStart) {
    auto task = engine::AsyncNoTracing([] { ADD_FAILURE() << "The task body should not start"; });
    task.RequestCancel();
    task.Wait();

    EXPECT_THAT(GetPlugin().GetSleepPendingFinalStates(), ::testing::ElementsAre(TaskState::kCancelled));
    EXPECT_THAT(GetPlugin().GetStopPendingFinalStates(), ::testing::ElementsAre(TaskState::kCancelled));
    EXPECT_EQ(task.GetState(), TaskState::kCancelled);
}

UTEST_F(EnginePluginPendingFinalState, CancelledInBody) {
    auto task = engine::AsyncNoTracing([] {
        EXPECT_EQ(engine::current_task::GetCurrentTaskContext().GetPendingFinalState(), TaskState::kInvalid);
        engine::Yield();
        engine::current_task::RequestCancel();
        engine::current_task::CancellationPoint();
    });
    task.Wait();

    EXPECT_THAT(
        GetPlugin().GetSleepPendingFinalStates(),
        ::testing::ElementsAre(TaskState::kInvalid, TaskState::kCancelled)
    );
    EXPECT_THAT(GetPlugin().GetStopPendingFinalStates(), ::testing::ElementsAre(TaskState::kCancelled));
    EXPECT_EQ(task.GetState(), TaskState::kCancelled);
}

USERVER_NAMESPACE_END
