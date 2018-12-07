#include <gtest/gtest.h>

#include <chrono>
#include <stdexcept>

#include <engine/async.hpp>
#include <engine/single_consumer_event.hpp>
#include <engine/sleep.hpp>
#include <engine/task/cancel.hpp>
#include <engine/task/task_context.hpp>
#include <engine/task/task_with_result.hpp>

#include <utest/utest.hpp>

// We do not want unwind to start in destructors
TEST(Cancel, NoUnwindFromDtor) {
  class SynchronizingRaii {
   public:
    SynchronizingRaii(engine::SingleConsumerEvent& request_event,
                      engine::SingleConsumerEvent& sync_event)
        : request_event_(request_event), sync_event_(sync_event) {}

    __attribute__((noinline)) ~SynchronizingRaii() {
      request_event_.Send();
      {
        engine::TaskCancellationBlocker block_cancel;
        EXPECT_TRUE(sync_event_.WaitForEvent());
      }
      EXPECT_NO_THROW(engine::current_task::CancellationPoint());
      EXPECT_TRUE(engine::current_task::IsCancelRequested());
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
    ASSERT_TRUE(in_dtor_event.WaitForEvent());
    task.RequestCancel();
    task.WaitFor(std::chrono::milliseconds(100));
    EXPECT_FALSE(task.IsFinished());
    cancel_sent_event.Send();
    task.Wait();
  });
}

// Functors defined in dtors should unwind though
TEST(Cancel, UnwindWorksInDtorSubtask) {
  class DetachingRaii {
   public:
    DetachingRaii(engine::SingleConsumerEvent& detach_event,
                  engine::TaskWithResult<void>& detached_task)
        : detach_event_(detach_event), detached_task_(detached_task) {}

    ~DetachingRaii() {
      detached_task_ = engine::Async([] {
        while (!engine::current_task::IsCancelRequested()) {
          engine::InterruptibleSleepFor(std::chrono::milliseconds(100));
        }
        engine::current_task::CancellationPoint();
        ADD_FAILURE() << "Cancelled task ran past cancellation point";
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
    ASSERT_TRUE(task_detached_event.WaitForEvent());
    task.Wait();

    detached_task.WaitFor(std::chrono::milliseconds(10));
    ASSERT_FALSE(detached_task.IsFinished());
    detached_task.RequestCancel();
    detached_task.Wait();
  });
}
