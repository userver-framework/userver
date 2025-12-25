#include <concurrent/impl/wait_wake.hpp>

#include <thread>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

constexpr std::size_t kTestThreadsCount = 100;

void GiveTimeToEnterSysCall() { std::this_thread::sleep_for(std::chrono::milliseconds(4)); }

TEST(WaitWake, SingleThread) {
    concurrent::impl::WaitWake ww;
    std::atomic<int> state = 0;
    std::atomic<bool> predicate_was_called = false;

    std::thread t{[&ww, &state, &predicate_was_called]() {
        ww.WaitByIndex(0, [&state, &predicate_was_called]() {
            if (!predicate_was_called.exchange(true)) {
                ++state;
            }
            return state.load() == 2;
        });
        EXPECT_EQ(state.load(), 2);
        ++state;
    }};

    while (state != 1) {
        std::this_thread::yield();
    }
    EXPECT_TRUE(predicate_was_called);
    state = 2;
    ww.WakeupAll();

    t.join();
    EXPECT_EQ(state.load(), 3);
}

TEST(WaitWake, WakeupTriggersPredicateRecheck) {
    concurrent::impl::WaitWake ww;
    std::atomic<int> state = 0;
    std::thread t{[&ww, &state]() {
        ww.WaitByIndex(0, [&state]() {
            ++state;
            return state.load() >= 2;
        });
    }};

    while (state != 1) {
        std::this_thread::yield();
    }

    // Make sure that wakeup not lost
    const auto woken_up = ww.WakeupAll();

    // Other thread may not have entered the OS sleep yet, but must wake up anyway
    EXPECT_LE(woken_up, 1);

    t.join();
    EXPECT_GE(state.load(), 2);
}

TEST(WaitWake, MultipleThreads) {
    concurrent::impl::WaitWake ww;
    std::atomic<int> state = 0;

    std::vector<std::thread> threads;
    threads.reserve(kTestThreadsCount);
    for (std::size_t i = 0; i < kTestThreadsCount; ++i) {
        threads.emplace_back([&ww, &state]() {
            ++state;
            ww.WaitByIndex(0, [&state]() { return state.load() == kTestThreadsCount + 1; });
            EXPECT_EQ(state.load(), kTestThreadsCount + 1);
        });
    }

    while (state != kTestThreadsCount) {
        std::this_thread::yield();
    }

    ++state;
    ww.WakeupAll();
    for (auto& t : threads) {
        t.join();
    }
}

#if __has_include(<linux/futex.h>)

TEST(WaitWake, FutexWakeupByIndex) {
    concurrent::impl::WaitWake ww;
    std::atomic<std::size_t> state = 0;

    std::vector<std::thread> threads;
    threads.reserve(kTestThreadsCount);
    for (std::size_t i = 0; i < kTestThreadsCount; ++i) {
        threads.emplace_back([i, &ww, &state]() {
            ++state;
            ww.WaitByIndex(i, [&state]() { return state.load() >= kTestThreadsCount + 1; });
            ++state;
        });
    }

    while (state != kTestThreadsCount) {
        std::this_thread::yield();
    }
    GiveTimeToEnterSysCall();

    ++state;
    const auto woken_up = ww.WakeupByIndex(0);

    // WaitWake distinguishes 32 indexes, but some may not entered the OS sleep yet or some spurious wakeups could
    // happen. Just checking that not all of the waiters were woken up
    EXPECT_LE(woken_up, kTestThreadsCount / 2);
    EXPECT_GE(woken_up, 1);  //  not guaranteed, but holds due to GiveTimeToEnterSysCall() and huge kTestThreadsCount

    // Give quite some time to wake up some of the affected
    do {
        std::this_thread::sleep_for(std::chrono::milliseconds(4));
    } while (state == kTestThreadsCount + 1);
    EXPECT_LT(state.load(), kTestThreadsCount * 2);

    for (std::size_t i = 1; i < kTestThreadsCount; ++i) {
        ww.WakeupByIndex(i);
    }

    for (auto& t : threads) {
        t.join();
    }
}

TEST(WaitWake, FutexWakeupSome) {
    concurrent::impl::WaitWake ww;
    std::atomic<std::size_t> state = 0;

    std::vector<std::thread> threads;
    threads.reserve(kTestThreadsCount);
    for (std::size_t i = 0; i < kTestThreadsCount; ++i) {
        threads.emplace_back([i, &ww, &state]() {
            ++state;
            ww.WaitByIndex(i, [&state]() { return state.load() >= kTestThreadsCount + 1; });
            ++state;
        });
    }

    while (state != kTestThreadsCount) {
        std::this_thread::yield();
    }
    GiveTimeToEnterSysCall();

    ++state;
    const int half_of_waiters{kTestThreadsCount / 2};
    const int woken_up = ww.WakeupSome(half_of_waiters);

    // WaitWake distinguishes 32 indexes, but some may not entered the OS sleep yet or some spurious wakeups could
    // happen. Just checking that not all of the waiters were woken up
    EXPECT_LE(woken_up, half_of_waiters);
    EXPECT_GE(woken_up, 1);  // not guaranteed, but holds due to GiveTimeToEnterSysCall() and huge kTestThreadsCount

    // Give quite some time to wake up all the affected
    do {
        std::this_thread::sleep_for(std::chrono::milliseconds(4));
    } while (state < kTestThreadsCount + half_of_waiters + 1);

    ww.WakeupSome(kTestThreadsCount - half_of_waiters);
    for (auto& t : threads) {
        t.join();
    }
}

#endif

USERVER_NAMESPACE_END
