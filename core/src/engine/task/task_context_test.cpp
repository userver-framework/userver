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
  ~DtorInCoroChecker() {
    EXPECT_TRUE(engine::current_task::IsTaskProcessorThread());
  }
};

// TAXICOMMON-4406 -- Wait list notification before cleanup gets lost
//
// The task had been woken up by a deadline timer and got notified by a wait
// list in the process of housekeeping. This notification was not accounted as a
// wakeup source because wakeup source was calculated early.
struct WaitListRaceSimulator final : public engine::impl::WaitStrategy {
  // cannot use passed deadline because of fast path
  WaitListRaceSimulator() : WaitStrategy{engine::Deadline{}} {}

  void SetupWakeups() override {
    // wake up immediately
    engine::current_task::GetCurrentTaskContext().Wakeup(
        engine::impl::TaskContext::WakeupSource::kDeadlineTimer,
        engine::impl::SleepState::Epoch{0});
  }

  void DisableWakeups() override {
    // simulate wait list notification before cleanup
    engine::current_task::GetCurrentTaskContext().Wakeup(
        engine::impl::TaskContext::WakeupSource::kWaitList,
        engine::impl::TaskContext::NoEpoch{});
  }
};

constexpr size_t kWorkerThreads = 1;

}  // namespace

UTEST_MT(TaskContext, DetachedAndCancelledOnStart, kWorkerThreads) {
  auto task = engine::AsyncNoSpan(
      [checker = DtorInCoroChecker()] { FAIL() << "Cancelled task started"; });
  task.RequestCancel();
  std::move(task).Detach();
}

UTEST_MT(TaskContext, DetachedAndCancelledOnStartWithWrappedCall,
         kWorkerThreads) {
  auto task = engine::AsyncNoSpan(
      [](auto&&) { FAIL() << "Cancelled task started"; }, DtorInCoroChecker());
  task.RequestCancel();
  std::move(task).Detach();
}

UTEST(TaskContext, WaitInterruptedReason) {
  auto long_task = engine::AsyncNoSpan(
      [] { engine::InterruptibleSleepFor(std::chrono::seconds{5}); });
  auto waiter = engine::AsyncNoSpan([&] {
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

  WaitListRaceSimulator wait_manager;
  EXPECT_EQ(context.Sleep(wait_manager),
            engine::impl::TaskContext::WakeupSource::kWaitList);
}

USERVER_NAMESPACE_END
