#include <userver/concurrent/impl/asymmetric_fence.hpp>

#include <atomic>
#include <cstddef>
#include <stdexcept>
#include <thread>

#include <userver/engine/async.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/fixed_array.hpp>

#include <concurrent/impl/asymmetric_fence_coalescer.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent::impl {

TEST(AsymmetricFence, CallbackExceptionsPropagateAndReleaseLock) {
    AsymmetricFenceCoalescer coalescer;
    const auto throwing = [] { throw std::runtime_error{"barrier failed"}; };
    const auto non_throwing = []() noexcept {};

    static_assert(!noexcept(coalescer.TryPerform(throwing)));
    static_assert(!noexcept(coalescer.Perform(throwing)));
    static_assert(!noexcept(coalescer.PerformPending(throwing)));
    static_assert(noexcept(coalescer.TryPerform(non_throwing)));
    static_assert(noexcept(coalescer.Perform(non_throwing)));
    static_assert(noexcept(coalescer.PerformPending(non_throwing)));

    EXPECT_THROW(coalescer.TryPerform(throwing), std::runtime_error);
    EXPECT_THROW(coalescer.Perform(throwing), std::runtime_error);
    coalescer.Enqueue();
    EXPECT_THROW(coalescer.PerformPending(throwing), std::runtime_error);
    EXPECT_TRUE(coalescer.TryPerform(non_throwing));
}

TEST(AsymmetricFence, CoalescesPendingRequests) {
    AsymmetricFenceCoalescer coalescer;
    int barriers = 0;
    const auto perform = [&barriers]() noexcept { ++barriers; };

    coalescer.Enqueue();
    coalescer.Enqueue();
    coalescer.PerformPending(perform);
    coalescer.PerformPending(perform);
    EXPECT_EQ(barriers, 1);

    coalescer.Enqueue();
    coalescer.PerformPending(perform);
    EXPECT_EQ(barriers, 2);
}

TEST(AsymmetricFence, RequestDuringBarrierNeedsAnotherBarrier) {
    AsymmetricFenceCoalescer coalescer;
    int barriers = 0;

    coalescer.Enqueue();
    coalescer.PerformPending([&coalescer, &barriers]() noexcept {
        ++barriers;
        coalescer.Enqueue();
    });
    coalescer.PerformPending([&barriers]() noexcept { ++barriers; });
    EXPECT_EQ(barriers, 2);
}

TEST(AsymmetricFence, ContendedTryPerformDoesNotBlock) {
    AsymmetricFenceCoalescer coalescer;
    std::atomic<bool> entered{false};
    std::atomic<bool> release{false};

    std::thread holder{[&coalescer, &entered, &release] {
        coalescer.Perform([&entered, &release]() noexcept {
            entered.store(true);
            while (!release.load()) {
                std::this_thread::yield();
            }
        });
    }};
    while (!entered.load()) {
        std::this_thread::yield();
    }

    EXPECT_FALSE(coalescer.TryPerform([]() noexcept { ADD_FAILURE(); }));
    release.store(true);
    holder.join();
    EXPECT_TRUE(coalescer.TryPerform([]() noexcept {}));
}

UTEST_MT(AsymmetricFence, ConcurrentTasksAndThreadsIgnoreCancellation, 4) {
    constexpr std::size_t kThreads = 2;
    constexpr std::size_t kTasks = 16;
    constexpr std::size_t kIterations = 1000;
    std::atomic<std::size_t> completed{0};

    auto threads = utils::GenerateFixedArray(kThreads, [&completed](std::size_t) {
        return std::thread{[&completed] {
            for (std::size_t i = 0; i < kIterations; ++i) {
                AsymmetricThreadFenceHeavy();
            }
            ++completed;
        }};
    });
    auto tasks = utils::GenerateFixedArray(kTasks, [&completed](std::size_t) {
        return engine::CriticalAsyncNoTracing([&completed] {
            engine::current_task::RequestCancel();
            for (std::size_t i = 0; i < kIterations; ++i) {
                AsymmetricThreadFenceHeavy();
                engine::Yield();
            }
            ++completed;
        });
    });

    for (auto& task : tasks) {
        UEXPECT_NO_THROW(task.Get());
    }
    for (auto& thread : threads) {
        thread.join();
    }
    EXPECT_EQ(completed.load(), kThreads + kTasks);
}

}  // namespace concurrent::impl

USERVER_NAMESPACE_END
