#include <userver/engine/pulse_event.hpp>

#include <atomic>
#include <thread>

#include <userver/engine/async.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/engine/task/current_task.hpp>
#include <userver/engine/wait_any.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/expected.hpp>
#include <userver/utils/fixed_array.hpp>

using namespace std::chrono_literals;

USERVER_NAMESPACE_BEGIN

UTEST(PulseEvent, UnusedEvent) { const engine::PulseEvent event; }

UTEST(PulseEvent, WaitUntilTimeoutKeepsSubscription) {
    engine::PulseEvent event;
    auto subscription = event.Subscribe();
    EXPECT_TRUE(subscription.IsValid());

    EXPECT_EQ(subscription.WaitUntil(engine::Deadline::Passed()), engine::FutureStatus::kTimeout);
    EXPECT_TRUE(subscription.IsValid());
    EXPECT_FALSE(subscription.IsReady());
}

UTEST(PulseEvent, SubscribeThenSendThenWait) {
    engine::PulseEvent event;
    auto subscription = event.Subscribe();

    event.Send();

    EXPECT_EQ(
        subscription.WaitUntil(engine::Deadline::FromDuration(utest::kMaxTestWaitTime)),
        engine::FutureStatus::kReady
    );
    EXPECT_TRUE(subscription.IsValid());
    EXPECT_TRUE(subscription.IsReady());
    EXPECT_EQ(subscription.WaitUntil(engine::Deadline::Passed()), engine::FutureStatus::kReady);
}

UTEST(PulseEvent, WaitAny) {
    engine::PulseEvent first;
    engine::PulseEvent second;
    auto first_subscription = first.Subscribe();
    auto second_subscription = second.Subscribe();

    auto waiter = engine::AsyncNoTracing([&] {
        auto wait_any = engine::MakeWaitAny(first_subscription, second_subscription);
        return wait_any.Wait();
    });

    engine::Yield();
    EXPECT_FALSE(waiter.IsFinished());

    second.Send();

    const auto ready = waiter.Get();
    ASSERT_TRUE(ready.has_value());
    EXPECT_EQ(*ready, 1);
    EXPECT_TRUE(second_subscription.IsReady());
    EXPECT_FALSE(first_subscription.IsReady());
}

UTEST(PulseEvent, SendIsNotSticky) {
    engine::PulseEvent event;
    event.Send();
    auto subscription = event.Subscribe();
    EXPECT_EQ(subscription.WaitUntil(engine::Deadline::Passed()), engine::FutureStatus::kTimeout);
}

UTEST(PulseEvent, WaitAndSend) {
    engine::PulseEvent event;
    auto task = engine::AsyncNoTracing([&] {
        auto subscription = event.Subscribe();
        EXPECT_EQ(subscription.WaitUntil({}), engine::FutureStatus::kReady);
    });

    engine::Yield();
    EXPECT_FALSE(task.IsFinished());

    event.Send();
    UEXPECT_NO_THROW(task.WaitFor(utest::kMaxTestWaitTime));
}

UTEST(PulseEvent, SendFromNonCoroutineThread) {
    engine::PulseEvent event;
    auto task = engine::AsyncNoTracing([&] {
        auto subscription = event.Subscribe();
        EXPECT_EQ(subscription.WaitUntil({}), engine::FutureStatus::kReady);
    });

    engine::Yield();
    EXPECT_FALSE(task.IsFinished());

    std::thread sender([&event] { event.Send(); });
    sender.join();

    UEXPECT_NO_THROW(task.WaitFor(utest::kMaxTestWaitTime));
}

UTEST(PulseEvent, SendDoesNotWakeFutureWaiters) {
    engine::PulseEvent event;
    event.Send();

    auto task = engine::AsyncNoTracing([&] {
        auto subscription = event.Subscribe();
        return subscription.WaitUntil(engine::Deadline::FromDuration(std::chrono::milliseconds{20}));
    });

    UEXPECT_NO_THROW(EXPECT_EQ(task.Get(), engine::FutureStatus::kTimeout));
}

UTEST(PulseEvent, MultipleWaiters) {
    engine::PulseEvent event;

    auto waiters = utils::GenerateFixedArray(4, [&](std::size_t) {
        return engine::AsyncNoTracing([&event] {
            auto subscription = event.Subscribe();
            EXPECT_EQ(subscription.WaitUntil({}), engine::FutureStatus::kReady);
        });
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

UTEST(PulseEvent, MultiplePulses) {
    engine::PulseEvent event;
    std::atomic<int> woke_up{0};

    auto waiter = engine::AsyncNoTracing([&] {
        for (int i = 0; i < 3; ++i) {
            auto subscription = event.Subscribe();
            EXPECT_EQ(subscription.WaitUntil({}), engine::FutureStatus::kReady);
            woke_up.fetch_add(1);
        }
    });

    for (int i = 0; i < 3; ++i) {
        while (woke_up.load() != i) {
            engine::Yield();
        }
        engine::Yield();
        event.Send();
    }

    UEXPECT_NO_THROW(waiter.WaitFor(utest::kMaxTestWaitTime));
    EXPECT_EQ(woke_up.load(), 3);
}

UTEST(PulseEvent, Deadline) {
    engine::PulseEvent event;
    auto subscription = event.Subscribe();

    EXPECT_EQ(subscription.WaitUntil(engine::Deadline::Passed()), engine::FutureStatus::kTimeout);
}

UTEST(PulseEvent, Cancellation) {
    engine::PulseEvent event;

    auto waiter = engine::CriticalAsyncNoTracing([&event] {
        auto subscription = event.Subscribe();
        const auto deadline = engine::Deadline::FromDuration(utest::kMaxTestWaitTime);
        EXPECT_EQ(subscription.WaitUntil(deadline), engine::FutureStatus::kCancelled);
        return engine::current_task::ShouldCancel();
    });

    waiter.SyncCancel();
    UEXPECT_NO_THROW(EXPECT_TRUE(waiter.Get()));
}

UTEST(PulseEvent, AlreadyCancelled) {
    engine::PulseEvent event;

    engine::current_task::RequestCancel();

    auto subscription = event.Subscribe();
    EXPECT_EQ(
        subscription.WaitUntil(engine::Deadline::FromDuration(utest::kMaxTestWaitTime)),
        engine::FutureStatus::kCancelled
    );
}

UTEST(PulseEvent, WaitUntilPredicate) {
    engine::PulseEvent event;
    std::atomic<bool> ready{false};

    auto waiter = engine::AsyncNoTracing([&] {
        return event.WaitUntil(engine::Deadline::FromDuration(utest::kMaxTestWaitTime), [&] { return ready.load(); });
    });

    engine::Yield();
    EXPECT_FALSE(waiter.IsFinished());

    ready.store(true);
    event.Send();

    UEXPECT_NO_THROW(EXPECT_EQ(waiter.Get(), engine::FutureStatus::kReady));
}

UTEST(PulseEvent, WaitUntilPredicateSendDuringCheck) {
    engine::PulseEvent event;
    std::atomic<bool> in_predicate{false};
    std::atomic<bool> ready{false};

    auto waiter = engine::AsyncNoTracing([&] {
        return event.WaitUntil(engine::Deadline::FromDuration(utest::kMaxTestWaitTime), [&] {
            in_predicate.store(true);
            return ready.load();
        });
    });

    while (!in_predicate.load()) {
        engine::Yield();
    }
    ready.store(true);
    event.Send();

    UEXPECT_NO_THROW(EXPECT_EQ(waiter.Get(), engine::FutureStatus::kReady));
}

UTEST(PulseEvent, WaitUntilPredicateAlreadyTrue) {
    engine::PulseEvent event;
    std::atomic<bool> ready{true};

    EXPECT_EQ(event.WaitUntil(engine::Deadline::Passed(), [&] { return ready.load(); }), engine::FutureStatus::kReady);
}

UTEST_MT(PulseEvent, ApplyConfigBroadcast, 3) {
    /// [WaitUntil subscription init]
    struct Config {
        int timeout_ms{};
        bool enabled{};

        bool operator==(const Config&) const = default;
    };

    std::atomic<int> apply_calls{0};
    const auto apply_config = [&apply_calls](const Config& /*config*/) {
        apply_calls.fetch_add(1, std::memory_order_relaxed);
    };

    std::atomic<int> timeout_ms{0};
    std::atomic<bool> enabled{false};
    engine::PulseEvent event;
    /// [WaitUntil subscription init]

    auto consumers = utils::GenerateFixedArray(2, [&](std::size_t) {
        return engine::CriticalAsyncNoTracing([&] {
            /// [WaitUntil subscription waiter]
            Config last{};
            while (!engine::current_task::ShouldCancel()) {
                auto subscription = event.Subscribe();
                const Config config{
                    .timeout_ms = timeout_ms.load(std::memory_order_relaxed),
                    .enabled = enabled.load(std::memory_order_relaxed),
                };
                if (config != last) {
                    apply_config(config);
                    last = config;
                    continue;
                }
                if (subscription.WaitUntil({}) != engine::FutureStatus::kReady) {
                    break;
                }
            }
            /// [WaitUntil subscription waiter]
        });
    });

    engine::Yield();

    /// [WaitUntil subscription notifier]
    timeout_ms.store(100, std::memory_order_relaxed);
    enabled.store(true, std::memory_order_relaxed);
    event.Send();
    /// [WaitUntil subscription notifier]

    while (apply_calls.load() < 2) {
        engine::Yield();
    }

    timeout_ms.store(200, std::memory_order_relaxed);
    enabled.store(false, std::memory_order_relaxed);
    event.Send();

    while (apply_calls.load() < 4) {
        engine::Yield();
    }

    for (auto& consumer : consumers) {
        consumer.RequestCancel();
        UEXPECT_NO_THROW(consumer.Get());
    }

    EXPECT_GE(apply_calls.load(), 4);
}

UTEST_MT(PulseEvent, AsConditionVariable, 3) {
    /// [CV init]
    std::atomic<bool> closed{true};
    engine::PulseEvent event;
    /// [CV init]

    auto waiters = utils::GenerateFixedArray(2, [&](std::size_t) {
        return engine::AsyncNoTracing([&] {
            /// [CV waiter]
            const auto wait_status = event.WaitUntil({}, [&] { return !closed.load(std::memory_order_relaxed); });
            /// [CV waiter]
            EXPECT_EQ(wait_status, engine::FutureStatus::kReady);
        });
    });

    engine::SleepFor(50ms);
    for (auto& waiter : waiters) {
        EXPECT_FALSE(waiter.IsFinished());
    }

    /// [CV notifier]
    closed.store(false, std::memory_order_relaxed);
    event.Send();
    /// [CV notifier]

    for (auto& waiter : waiters) {
        UEXPECT_NO_THROW(waiter.Get());
    }
}

USERVER_NAMESPACE_END
