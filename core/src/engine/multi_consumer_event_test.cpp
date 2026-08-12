#include <userver/engine/multi_consumer_event.hpp>

#include <atomic>
#include <optional>
#include <vector>

#include <userver/engine/async.hpp>
#include <userver/engine/exception.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/engine/task/current_task.hpp>
#include <userver/engine/wait_any.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/fixed_array.hpp>

using namespace std::chrono_literals;

USERVER_NAMESPACE_BEGIN

UTEST(MultiConsumerEvent, UnusedEvent) { const engine::MultiConsumerEvent event; }

UTEST(MultiConsumerEvent, IsReady) {
    engine::MultiConsumerEvent event;
    EXPECT_FALSE(event.IsReady());
    event.Send();
    EXPECT_TRUE(event.IsReady());
    EXPECT_TRUE(event.IsReady());

    EXPECT_EQ(event.WaitUntil(engine::Deadline{}), engine::FutureStatus::kReady);
    EXPECT_TRUE(event.IsReady());
}

UTEST(MultiConsumerEvent, WaitAndSend) {
    engine::MultiConsumerEvent event;
    auto task = engine::AsyncNoTracing([&] { UEXPECT_NO_THROW(event.Wait()); });

    engine::Yield();
    EXPECT_FALSE(task.IsFinished());

    event.Send();
    UEXPECT_NO_THROW(task.WaitFor(utest::kMaxTestWaitTime));
}

UTEST(MultiConsumerEvent, SendAndWait) {
    engine::MultiConsumerEvent event;
    std::atomic<bool> is_event_sent{false};

    auto task = engine::AsyncNoTracing([&] {
        while (!is_event_sent) {
            engine::Yield();
        }
        UEXPECT_NO_THROW(event.Wait());
    });

    event.Send();
    is_event_sent = true;

    UEXPECT_NO_THROW(task.WaitFor(utest::kMaxTestWaitTime));
}

UTEST(MultiConsumerEvent, MultipleWaiters) {
    engine::MultiConsumerEvent event;

    auto waiters = utils::GenerateFixedArray(4, [&](std::size_t) {
        return engine::AsyncNoTracing([&event] { UEXPECT_NO_THROW(event.Wait()); });
    });

    engine::Yield();
    for (auto& waiter : waiters) {
        EXPECT_FALSE(waiter.IsFinished());
    }

    event.Send();

    for (auto& waiter : waiters) {
        UEXPECT_NO_THROW(waiter.WaitFor(utest::kMaxTestWaitTime));
        UEXPECT_NO_THROW(waiter.Get());
    }
}

UTEST(MultiConsumerEvent, Deadline) {
    engine::MultiConsumerEvent event;

    EXPECT_EQ(event.WaitUntil(engine::Deadline::Passed()), engine::FutureStatus::kTimeout);
    EXPECT_FALSE(event.IsReady());
}

UTEST(MultiConsumerEvent, Cancellation) {
    engine::MultiConsumerEvent event;

    auto waiter = engine::CriticalAsyncNoTracing([&event] {
        const auto deadline = engine::Deadline::FromDuration(utest::kMaxTestWaitTime);
        EXPECT_EQ(event.WaitUntil(deadline), engine::FutureStatus::kCancelled);
        UEXPECT_THROW(event.Wait(), engine::WaitInterruptedException);
        return engine::current_task::ShouldCancel();
    });

    waiter.SyncCancel();
    UEXPECT_NO_THROW(EXPECT_TRUE(waiter.Get()));
}

UTEST(MultiConsumerEvent, AlreadyCancelled) {
    engine::MultiConsumerEvent event;

    engine::current_task::RequestCancel();

    EXPECT_EQ(
        event.WaitUntil(engine::Deadline::FromDuration(utest::kMaxTestWaitTime)),
        engine::FutureStatus::kCancelled
    );
    UEXPECT_THROW(event.Wait(), engine::WaitInterruptedException);
}

UTEST(MultiConsumerEvent, WaitAnyCancellation) {
    engine::MultiConsumerEvent event1;
    engine::MultiConsumerEvent event2;

    auto waiter = engine::CriticalAsyncNoTracing([&] {
        return engine::WaitAnyUntil(engine::Deadline::FromDuration(utest::kMaxTestWaitTime), event1, event2);
    });

    waiter.SyncCancel();
    UEXPECT_NO_THROW(EXPECT_EQ(waiter.Get(), std::nullopt));
}

USERVER_NAMESPACE_END
