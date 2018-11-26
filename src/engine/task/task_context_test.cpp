#include <gtest/gtest.h>

#include <chrono>
#include <stdexcept>

#include <engine/async.hpp>
#include <engine/single_consumer_event.hpp>
#include <engine/sleep.hpp>
#include <engine/task/task_context.hpp>
#include <engine/task/task_with_result.hpp>

#include <utest/utest.hpp>

// TAXICOMMON-347 -- Task closure gets destroyed in TaskProcessor::ProcessTasks.
//
// The task was detached (no coroutine ownership) and it got cancelled as
// the processor shut down. As the task hasn't started yet, we invoked the fast
// path that did not acquire a coroutine, and payload was destroyed in
// an unexpected context.
TEST(TaskContext, DetachedAndCancelledOnStart) {
  class DtorInCoroChecker {
   public:
    ~DtorInCoroChecker() {
      EXPECT_NE(nullptr,
                engine::current_task::GetCurrentTaskContextUnchecked());
    }
  };

  static constexpr size_t kWorkerThreads = 1;

  RunInCoro(
      [] {
        auto task = engine::Async([checker = DtorInCoroChecker()] {
          FAIL() << "Cancelled task started";
        });
        task.RequestCancel();
        std::move(task).Detach();
      },
      kWorkerThreads);

  // Same, but with WrappedCall closure
  RunInCoro(
      [] {
        auto task =
            engine::Async([](auto&&) { FAIL() << "Cancelled task started"; },
                          DtorInCoroChecker());
        task.RequestCancel();
        std::move(task).Detach();
      },
      kWorkerThreads);
}

// We do not want unwind to start in destructors
TEST(TaskContext, NoUnwindFromDtor) {
  class SynchronizingRaii {
   public:
    SynchronizingRaii(engine::SingleConsumerEvent& request_event,
                      engine::SingleConsumerEvent& sync_event)
        : request_event_(request_event), sync_event_(sync_event) {}

    __attribute__((noinline)) ~SynchronizingRaii() {
      request_event_.Send();
      // ASSERT_NO_THROW uses return <expr> internally, does not compile
      EXPECT_NO_THROW(sync_event_.WaitForEvent());
    }

   private:
    engine::SingleConsumerEvent& request_event_;
    engine::SingleConsumerEvent& sync_event_;
  };

  RunInCoro([] {
    engine::SingleConsumerEvent in_dtor_event;
    engine::SingleConsumerEvent cancel_sent_event;
    auto task = engine::Async(
        [&] { SynchronizingRaii raii(in_dtor_event, cancel_sent_event); });
    in_dtor_event.WaitForEvent();
    task.RequestCancel();
    task.WaitFor(std::chrono::milliseconds(100));
    EXPECT_FALSE(task.IsFinished());
    cancel_sent_event.Send();
    task.Wait();
  });
}

// Functors defined in dtors should unwind though
TEST(TaskContext, UnwindWorksInDtorSubtask) {
  class DetachingRaii {
   public:
    DetachingRaii(engine::SingleConsumerEvent& detach_event,
                  engine::TaskWithResult<void>& detached_task)
        : detach_event_(detach_event), detached_task_(detached_task) {}

    ~DetachingRaii() {
      detached_task_ = engine::Async([] {
        while (true) {
          engine::SleepFor(std::chrono::milliseconds(100));
          engine::current_task::CancellationPoint();
        }
      });
      detach_event_.Send();
    }

   private:
    engine::SingleConsumerEvent& detach_event_;
    engine::TaskWithResult<void>& detached_task_;
  };

  RunInCoro([] {
    engine::TaskWithResult<void> detached_task;
    engine::SingleConsumerEvent task_detached_event;
    auto task = engine::Async(
        [&] { DetachingRaii raii(task_detached_event, detached_task); });
    task_detached_event.WaitForEvent();
    task.Wait();

    ASSERT_FALSE(detached_task.IsFinished());
    detached_task.RequestCancel();
    detached_task.Wait();
  });
}
