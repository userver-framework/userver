#include <gtest/gtest.h>

#include <engine/task/sleep_state.hpp>
#include <engine/task/task_context.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/sleep.hpp>

#include <userver/utest/utest.hpp>

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
    EXPECT_NE(nullptr, engine::current_task::GetCurrentTaskContextUnchecked());
  }
};

// TAXICOMMON-4208 -- Wait list sends a notification before cleanup
//
// The task had been woken up by a deadline timer and got notified by a wait
// list in the process of housekeeping. This notification was not cleared
// properly and became the reason of the next (unexpected) wakeup.
struct WaitListRaceSimulator final : public engine::impl::WaitStrategy {
  // cannot use passed deadline because of fast path
  WaitListRaceSimulator() : WaitStrategy{engine::Deadline{}} {}

  void AfterAsleep() override {
    // wake up immediately
    engine::current_task::GetCurrentTaskContext()->Wakeup(
        engine::impl::TaskContext::WakeupSource::kDeadlineTimer,
        engine::impl::SleepState::Epoch{0});
  }

  void BeforeAwake() override {
    // simulate wait list notification before cleanup
    engine::current_task::GetCurrentTaskContext()->Wakeup(
        engine::impl::TaskContext::WakeupSource::kWaitList,
        engine::impl::TaskContext::NoEpoch{});
  }
};

static constexpr size_t kWorkerThreads = 1;

}  // namespace

UTEST_MT(TaskContext, DetachedAndCancelledOnStart, kWorkerThreads) {
  auto task = engine::impl::Async(
      [checker = DtorInCoroChecker()] { FAIL() << "Cancelled task started"; });
  task.RequestCancel();
  std::move(task).Detach();
}

UTEST_MT(TaskContext, DetachedAndCancelledOnStartWithWrappedCall,
         kWorkerThreads) {
  auto task = engine::impl::Async(
      [](auto&&) { FAIL() << "Cancelled task started"; }, DtorInCoroChecker());
  task.RequestCancel();
  std::move(task).Detach();
}

UTEST(TaskContext, WaitInterruptedReason) {
  auto long_task = engine::impl::Async(
      [] { engine::InterruptibleSleepFor(std::chrono::seconds{5}); });
  auto waiter = engine::impl::Async([&] {
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
  auto* const context = engine::current_task::GetCurrentTaskContext();

  // init
  WaitListRaceSimulator wait_manager;
  context->Sleep(wait_manager);

  // wake up with non-wait-list reason
  engine::Yield();

  EXPECT_NE(context->DebugGetWakeupSource(),
            engine::impl::TaskContext::WakeupSource::kWaitList);
}
