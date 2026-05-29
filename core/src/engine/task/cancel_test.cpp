#include <userver/engine/task/cancel.hpp>

#include <chrono>
#include <stdexcept>

#include <fmt/format.h>

#include <userver/engine/async.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/future.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/single_use_event.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/fast_scope_guard.hpp>
#include <userver/utils/task_builder.hpp>

using namespace std::chrono_literals;

USERVER_NAMESPACE_BEGIN

// Functors defined in dtors should unwind though
UTEST(Cancel, UnwindWorksInDtorSubtask) {
    class DetachingRaii final {
    public:
        DetachingRaii(engine::SingleConsumerEvent& detach_event, engine::TaskWithResult<void>& detached_task)
            : detach_event_(detach_event),
              detached_task_(detached_task)
        {}

        ~DetachingRaii() {
            detached_task_ = engine::AsyncNoTracing([] {
                engine::InterruptibleSleepFor(utest::kMaxTestWaitTime);
                engine::current_task::CancellationPoint();
                ADD_FAILURE() << "Cancelled task ran past cancellation point";
            });
            detach_event_.Send();
        }

    private:
        engine::SingleConsumerEvent& detach_event_;
        engine::TaskWithResult<void>& detached_task_;
    };

    engine::TaskWithResult<void> detached_task;
    engine::SingleConsumerEvent task_detached_event;
    auto task = engine::AsyncNoTracing([&] { const DetachingRaii raii(task_detached_event, detached_task); });
    ASSERT_TRUE(task_detached_event.WaitForEvent());
    task.Wait();

    detached_task.WaitFor(10ms);
    ASSERT_FALSE(detached_task.IsFinished());
    detached_task.SyncCancel();
}

UTEST(Cancel, CancelDuringInterruptibleSleep) {
    engine::SingleConsumerEvent task_started;

    auto task = engine::CriticalAsyncNoTracing([&] {
        EXPECT_FALSE(engine::current_task::IsCancelRequested());
        task_started.Send();

        engine::InterruptibleSleepFor(utest::kMaxTestWaitTime);
        EXPECT_TRUE(engine::current_task::ShouldCancel());
    });

    EXPECT_TRUE(task_started.WaitForEventFor(utest::kMaxTestWaitTime));
    task.RequestCancel();
    UEXPECT_NO_THROW(task.Get());
}

UTEST(Cancel, CancelBeforeInterruptibleSleep) {
    engine::current_task::GetCancellationToken().RequestCancel();

    // The task should wake up from this sleep immediately, because it is
    // already cancelled
    engine::InterruptibleSleepFor(utest::kMaxTestWaitTime);
    EXPECT_TRUE(engine::current_task::ShouldCancel());
}

namespace {

constexpr int kTaskResult = 42;

// Can be practically anything, but should be large enough to (most of the time)
// encompass all the CPU work
constexpr auto kSmallDuration = 10ms;

void CheckDeadlineCancelled(engine::TaskWithResult<void>& task) {
    UASSERT_NO_THROW(task.WaitFor(utest::kMaxTestWaitTime / 2));
    EXPECT_EQ(task.GetState(), engine::Task::State::kCancelled);
    EXPECT_EQ(task.CancellationReason(), engine::TaskCancellationReason::kDeadline);
    UEXPECT_THROW(task.Get(), engine::TaskCancelledException);
}

void CheckDeadlineCompleted(engine::TaskWithResult<int>& task) {
    UASSERT_NO_THROW(task.WaitFor(utest::kMaxTestWaitTime / 2));
    // Despite being cancelled, the task exited in a user-defined manner
    EXPECT_EQ(task.GetState(), engine::Task::State::kCompleted);
    EXPECT_EQ(task.CancellationReason(), engine::TaskCancellationReason::kDeadline);
    UEXPECT_NO_THROW(EXPECT_EQ(task.Get(), kTaskResult));
}

void CheckNoDeadlineCompleted(engine::TaskWithResult<int>& task) {
    UASSERT_NO_THROW(task.WaitFor(utest::kMaxTestWaitTime / 2));
    EXPECT_EQ(task.GetState(), engine::Task::State::kCompleted);
    EXPECT_EQ(task.CancellationReason(), engine::TaskCancellationReason::kNone);
    UEXPECT_NO_THROW(EXPECT_EQ(task.Get(), kTaskResult));
}

void CheckUserCancelled(engine::TaskWithResult<int>& task) {
    UASSERT_NO_THROW(task.WaitFor(utest::kMaxTestWaitTime / 2));
    EXPECT_EQ(task.GetState(), engine::Task::State::kCancelled);
    EXPECT_EQ(task.CancellationReason(), engine::TaskCancellationReason::kUserRequest);
    UEXPECT_THROW(task.Get(), engine::TaskCancelledException);
}

}  // namespace

UTEST(Cancel, DeadlineBeforeTaskStarted) {
    const auto passed_deadline = engine::Deadline::FromDuration(-1s);

    auto task = utils::TaskBuilder{}.NoSpan().Background().Deadline(passed_deadline).Build([] {
        FAIL() << "This task's body should not run";
    });

    CheckDeadlineCancelled(task);
}

UTEST(Cancel, DeadlineBeforeTaskStartedCritical) {
    const auto passed_deadline = engine::Deadline::FromDuration(-1s);
    const engine::SingleConsumerEvent infinity;

    auto task = utils::TaskBuilder{}.NoSpan().Background().Critical().Deadline(passed_deadline).Build([&] {
        // Critical should ensure that the task is started, but should still allow
        // the cancellations to work within the task body.
        EXPECT_TRUE(engine::current_task::ShouldCancel());
        return kTaskResult;
    });

    CheckDeadlineCompleted(task);
}

UTEST(Cancel, DeadlineShouldCancel) {
    const auto deadline = engine::Deadline::FromDuration(kSmallDuration);

    auto task = utils::TaskBuilder{}.NoSpan().Background().Critical().Deadline(deadline).Build([] {
        while (!engine::current_task::ShouldCancel()) {
            // Normally, some CPU-bound work should go here
        }
        return kTaskResult;
    });

    CheckDeadlineCompleted(task);
}

UTEST(Cancel, DeadlineCancellationPoint) {
    const auto deadline = engine::Deadline::FromDuration(kSmallDuration);

    auto task = utils::TaskBuilder{}.NoSpan().Background().Critical().Deadline(deadline).Build([] {
        while (true) {
            engine::current_task::CancellationPoint();
            // Normally, some CPU-bound work should go here
        }
        FAIL() << "A stack-unwinding exception should be thrown";
    });

    CheckDeadlineCancelled(task);
}

UTEST(Cancel, DeadlineNotReached) {
    const auto deadline = engine::Deadline::FromDuration(utest::kMaxTestWaitTime);
    engine::SingleConsumerEvent delayed_event;
    engine::SingleConsumerEvent infinity;

    auto task = utils::TaskBuilder{}.NoSpan().Background().Deadline(deadline).Build([&] {
        EXPECT_FALSE(infinity.WaitForEventFor(kSmallDuration));
        EXPECT_FALSE(engine::current_task::IsCancelRequested());

        EXPECT_TRUE(delayed_event.WaitForEvent());
        EXPECT_FALSE(engine::current_task::IsCancelRequested());
        return kTaskResult;
    });

    engine::SleepFor(kSmallDuration);
    delayed_event.Send();

    CheckNoDeadlineCompleted(task);
}

UTEST(Cancel, SetDeadline) {
    engine::SingleConsumerEvent delayed_event;
    engine::SingleConsumerEvent infinity;

    auto task = engine::AsyncNoTracing([&] {
        engine::current_task::SetDeadline(engine::Deadline::FromDuration(utest::kMaxTestWaitTime));

        // This wait should succeed without reaching the task deadline
        EXPECT_TRUE(delayed_event.WaitForEvent());
        EXPECT_FALSE(engine::current_task::ShouldCancel());

        engine::current_task::SetDeadline(engine::Deadline::FromDuration(kSmallDuration));

        // This wait should be interrupted by cancellation due to deadline
        EXPECT_FALSE(infinity.WaitForEventFor(utest::kMaxTestWaitTime));
        EXPECT_TRUE(engine::current_task::ShouldCancel());
        return kTaskResult;
    });

    engine::SleepFor(kSmallDuration);
    delayed_event.Send();

    CheckDeadlineCompleted(task);
}

UTEST(Cancel, CancellationBlocker) {
    engine::SingleConsumerEvent delayed_event;
    engine::SingleConsumerEvent infinity;

    auto task = engine::AsyncNoTracing([&] {
        engine::current_task::SetDeadline(engine::Deadline::FromDuration(-1s));

        {
            const engine::TaskCancellationBlocker cancel_blocker;
            EXPECT_TRUE(delayed_event.WaitForEventFor(utest::kMaxTestWaitTime));
            EXPECT_FALSE(engine::current_task::ShouldCancel());
        }

        EXPECT_FALSE(infinity.WaitForEventFor(utest::kMaxTestWaitTime));
        EXPECT_TRUE(engine::current_task::ShouldCancel());
        return kTaskResult;
    });

    engine::SleepFor(kSmallDuration);
    delayed_event.Send();

    CheckDeadlineCompleted(task);
}

UTEST(Cancel, DeadlinePropagationParentToChild) {
    const auto deadline = engine::Deadline::FromDuration(kSmallDuration);
    bool wait_interrupted_exception_thrown = false;
    bool child_finished_ok = false;

    auto parent_task = utils::TaskBuilder{}.NoSpan().Background().Critical().Deadline(deadline).Build([&] {
        try {
            auto child_task = engine::CriticalAsyncNoTracing([&] {
                engine::InterruptibleSleepUntil(engine::Deadline::FromDuration(utest::kMaxTestWaitTime));
                child_finished_ok = true;
            });

            // When task cancellation deadline hits the parent task, the wait should
            // be interrupted with an exception. During stack unwinding the child task
            // will typically be cancelled and waited for.
            child_task.Wait();
        } catch (const engine::WaitInterruptedException& ex) {
            wait_interrupted_exception_thrown = true;
            EXPECT_TRUE(child_finished_ok);
            throw;
        }
    });

    UEXPECT_NO_THROW(parent_task.WaitFor(utest::kMaxTestWaitTime));
    EXPECT_EQ(parent_task.CancellationReason(), engine::TaskCancellationReason::kDeadline);
    UEXPECT_THROW(parent_task.Get(), engine::WaitInterruptedException);
}

UTEST(Cancel, DeadlinePropagationNotChildToParent) {
    const auto deadline = engine::Deadline::FromDuration(kSmallDuration);
    engine::Promise<int> promise;

    auto child_task =
        utils::TaskBuilder{}
            .NoSpan()
            .Background()
            .Critical()
            .Deadline(deadline)
            .Build([&, future = promise.get_future()]() mutable { return future.get(); });

    // Deadline set for a child task does not affect the parent task directly.
    // However, it is expected that the child task will signal the failure in some
    // way - in this case it throws an exception. The parent task can then choose
    // to propagate the failure, or to ignore it.
    UEXPECT_NO_THROW(child_task.WaitFor(utest::kMaxTestWaitTime));
    EXPECT_EQ(child_task.CancellationReason(), engine::TaskCancellationReason::kDeadline);
    UEXPECT_THROW(child_task.Get(), engine::WaitInterruptedException);
    EXPECT_FALSE(engine::current_task::IsCancelRequested());
}

UTEST(Cancel, CancellationTokenRequestCancel) {
    engine::SingleConsumerEvent event;
    auto task = engine::CriticalAsyncNoTracing([&event] {
        EXPECT_FALSE(event.WaitForEvent());
        engine::current_task::CancellationPoint();
        return kTaskResult;
    });

    engine::TaskCancellationToken token{task};

    token.RequestCancel();

    CheckUserCancelled(task);
}

UTEST(Cancel, CancellationTokenDtorNoWait) {
    engine::SingleConsumerEvent event;
    auto task = engine::CriticalAsyncNoTracing([&event] {
        EXPECT_TRUE(event.WaitForEvent());
        return kTaskResult;
    });

    {
        const engine::TaskCancellationToken token{task};
    }
    // destroying token neither waits for task, nor
    // cancels the task
    EXPECT_EQ(engine::TaskCancellationReason::kNone, task.CancellationReason());

    // By special request from reviewer, this time we let the task complete
    // successfully
    event.Send();

    EXPECT_EQ(kTaskResult, task.Get());
}

UTEST(Cancel, CancellationTokenCancelSelf) {
    auto task = engine::CriticalAsyncNoTracing([] {
        auto token = engine::current_task::GetCancellationToken();
        token.RequestCancel();
        EXPECT_TRUE(token.IsCancelRequested());
        EXPECT_TRUE(token.CancellationReason() == engine::TaskCancellationReason::kUserRequest);
        engine::current_task::CancellationPoint();
        return kTaskResult;
    });

    CheckUserCancelled(task);
}

UTEST(Cancel, CancellationTokenLifetime) {
    // Check that token can outlive task
    engine::TaskCancellationToken token;

    {
        auto task = engine::CriticalAsyncNoTracing([] { return kTaskResult; });

        token = engine::TaskCancellationToken{task};
        EXPECT_TRUE(token.IsValid());

        EXPECT_EQ(kTaskResult, task.Get());
    }

    EXPECT_TRUE(token.IsValid());
    token.RequestCancel();
}

UTEST(Cancel, CriticalSample) {
    /// [critical cancel]
    bool task_was_run = false;
    auto task = utils::CriticalAsync("sleep", [&task_was_run] {
        task_was_run = true;
        engine::InterruptibleSleepFor(utest::kMaxTestWaitTime);
    });

    task.RequestCancel();

    // It will actually typically only take a few microseconds for the task to complete.
    task.WaitFor(utest::kMaxTestWaitTime / 2);
    // Check that the cancellation interrupted the sleep.
    EXPECT_TRUE(task.IsFinished());
    UEXPECT_NO_THROW(task.Get());

    EXPECT_TRUE(task_was_run);
    /// [critical cancel]
}

namespace {
namespace drop_task_by_unwind {

/// [stack unwinding destroys task]
void Child() {
    engine::InterruptibleSleepFor(utest::kMaxTestWaitTime);
    ASSERT_TRUE(engine::current_task::ShouldCancel());
    throw std::runtime_error("This exception will be swallowed in the task destructor");
}

void SomeOtherWork() { throw std::runtime_error("Something went wrong"); }

void Parent() {
    auto child_task = utils::Async("child", Child);
    // Now the current function proceeds to do some other work. Suppose it throws an exception.
    // `child_task` is destroyed during stack unwinding, and the destructor
    // cancels and awaits `child_task`. Its exception is swallowed in the destructor.
    SomeOtherWork();
    // After we've done our work, we'd expect to merge in child_task's result.
    child_task.Get();
}
/// [stack unwinding destroys task]

}  // namespace drop_task_by_unwind
}  // namespace

UTEST(Cancel, DropTaskByUnwindSample) {
    const auto deadline = engine::Deadline::FromDuration(utest::kMaxTestWaitTime / 2);
    UEXPECT_THROW_MSG(drop_task_by_unwind::Parent(), std::runtime_error, "Something went wrong");
    // Check that the cancellation worked on InterruptibleSleepFor in Foo.
    EXPECT_FALSE(deadline.IsReached());
}

namespace {
namespace parent_cancelled {

/// [parent cancelled]
void Child() { engine::InterruptibleSleepFor(utest::kMaxTestWaitTime); }

void Parent() {
    auto child_task = utils::Async("child", Child);
    // Cancel ourselves for the sake of a simple example. On practice, Parent's parent will cancel it.
    // The cancellation will be visible at the next waiting operation.
    engine::current_task::RequestCancel();

    try {
        child_task.Get();
        FAIL() << "The line above should have thrown";
    } catch (const engine::WaitInterruptedException& /*ex*/) {
        // Cancelling Parent does not magically cancel any other tasks...
        engine::SleepFor(std::chrono::milliseconds{10});
        EXPECT_FALSE(child_task.IsFinished());
        throw;
    }

    // ...It typically happens because `child_task` exits the scope.
}
/// [parent cancelled]

}  // namespace parent_cancelled
}  // namespace

UTEST(Cancel, ParentCancelledSample) {
    const auto deadline = engine::Deadline::FromDuration(utest::kMaxTestWaitTime / 2);
    UEXPECT_THROW(parent_cancelled::Parent(), engine::WaitInterruptedException);
    // Check that the cancellation worked on InterruptibleSleepFor in Child.
    EXPECT_FALSE(deadline.IsReached());
}

UTEST(Async, CancellationBeforeStartNormal) {
    // The test relies on there only being a single TaskProcessor thread, otherwise there could be a race
    // between `RequestCancel()` and the concurrent execution of the child task.
    ASSERT_EQ(GetThreadCount(), 1);

    bool captures_destroyed = false;
    utils::FastScopeGuard guard([&captures_destroyed]() noexcept {
        EXPECT_TRUE(engine::current_task::IsTaskProcessorThread());
        captures_destroyed = true;
    });

    auto task = engine::AsyncNoTracing([guard = std::move(guard)] { ADD_FAILURE() << "The task body should not start"; }
    );

    EXPECT_FALSE(task.IsFinished());
    EXPECT_FALSE(captures_destroyed);

    // The task has not started by this point. This means that the task is cancelled before starting.
    // This should cause the task body to be skipped, because this is a non-Critical task.
    // See details on engine::TaskBase::State::kCancelled.
    task.RequestCancel();

    task.WaitFor(utest::kMaxTestWaitTime);
    EXPECT_EQ(task.GetState(), engine::TaskBase::State::kCancelled);
    UEXPECT_THROW(task.Get(), engine::TaskCancelledException);
    // The task destructor should run even if the execution of the task body is skipped due to cancellation.
    EXPECT_TRUE(captures_destroyed);
}

UTEST(Async, CancellationBeforeStartCritical) {
    // The test relies on there only being a single TaskProcessor thread, otherwise there could be a race
    // between `RequestCancel()` and the concurrent execution of the child task.
    ASSERT_EQ(GetThreadCount(), 1);

    engine::SingleUseEvent event;
    auto task = engine::CriticalAsyncNoTracing([&event] {
        // The task should start (proven by `return true;` below and `Get() -> true`.)

        EXPECT_TRUE(engine::current_task::impl::IsCritical());

        // The task is still cancelled, and any waiting operation will be interrupted.
        EXPECT_TRUE(engine::current_task::IsCancelRequested());
        EXPECT_TRUE(engine::current_task::ShouldCancel());
        const auto deadline = engine::Deadline::FromDuration(utest::kMaxTestWaitTime);
        EXPECT_EQ(event.WaitUntil(deadline), engine::FutureStatus::kCancelled);

        return true;
    });

    EXPECT_FALSE(task.IsFinished());

    // The task has not started by this point. This means that the task is cancelled before starting.
    // But because the task is started as Critical, the task body should be executed anyway.
    task.RequestCancel();

    task.WaitFor(utest::kMaxTestWaitTime / 2);
    EXPECT_EQ(task.GetState(), engine::TaskBase::State::kCompleted);
    EXPECT_TRUE(task.Get());
}

USERVER_NAMESPACE_END
