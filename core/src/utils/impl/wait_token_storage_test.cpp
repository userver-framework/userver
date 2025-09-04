#include <userver/utils/impl/wait_token_storage.hpp>

#include <atomic>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <userver/engine/async.hpp>
#include <userver/engine/run_standalone.hpp>
#include <userver/engine/shared_mutex.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/fixed_array.hpp>

using namespace std::chrono_literals;
using namespace std::string_literals;

USERVER_NAMESPACE_BEGIN

UTEST(WaitTokenStorage, NoTokens) {
    utils::impl::WaitTokenStorage wts;
    EXPECT_EQ(wts.AliveTokensApprox(), 0);
    wts.WaitForAllTokens();
}

UTEST(WaitTokenStorage, NoTokensNoWait) {
    try {
        const utils::impl::WaitTokenStorage wts;

        // Scenario: the constructor of the WTS owner throws, so its destructor (and
        // WaitForAllTokens) is not called.
        throw std::runtime_error("test");
    } catch (const std::runtime_error& ex) {
        EXPECT_EQ(ex.what(), "test"s);
    }
}

UTEST(WaitTokenStorage, SingleToken) {
    utils::impl::WaitTokenStorage wts;
    std::atomic<bool> is_finished{false};

    auto task = engine::AsyncNoSpan([&, token = wts.GetToken()] {
        engine::SleepFor(50ms);
        is_finished = true;
    });

    wts.WaitForAllTokens();
    EXPECT_TRUE(is_finished);
}

UTEST_MT(WaitTokenStorage, MultipleTokens, 4) {
    constexpr std::size_t kLauncherCount = 2;
    constexpr std::size_t kWorkersPerLauncher = 100;
    constexpr std::size_t kTaskCount = kLauncherCount * kWorkersPerLauncher;

    utils::impl::WaitTokenStorage wts;
    std::atomic<int> workers_completed{0};
    engine::SharedMutex allowed_to_finish;
    std::unique_lock allowed_to_finish_lock(allowed_to_finish);

    std::vector<engine::TaskWithResult<void>> launcher_tasks;
    launcher_tasks.reserve(kLauncherCount);

    for (std::size_t i = 0; i < kLauncherCount; ++i) {
        launcher_tasks.push_back(engine::AsyncNoSpan([&] {
            // Give all the launcher tasks time to start before the TaskProcessor is
            // clobbered by the detached tasks
            engine::SleepFor(1ms);

            for (std::size_t j = 0; j < kWorkersPerLauncher; ++j) {
                // Note: the token is created in one task and moved into another one
                engine::DetachUnscopedUnsafe(engine::AsyncNoSpan([&, token = wts.GetToken()] {
                    const std::shared_lock allowed_to_finish_lock(allowed_to_finish);
                    ++workers_completed;
                }));
            }
        }));
    }

    for (auto& task : launcher_tasks) task.Get();
    EXPECT_EQ(wts.AliveTokensApprox(), kTaskCount);
    allowed_to_finish_lock.unlock();

    // No more tasks must be launched at this point
    wts.WaitForAllTokens();
    EXPECT_EQ(workers_completed, kTaskCount);
}

UTEST_MT(WaitTokenStorage, TokenReleaseRace, 3) {
    static constexpr std::size_t kTokenOwnerCount = 2;
    static constexpr std::chrono::milliseconds kTestDuration{300};
    const auto test_deadline = engine::Deadline::FromDuration(kTestDuration);

    while (!test_deadline.IsReached()) {
        utils::impl::WaitTokenStorage wts;
        std::atomic<int> allowed_to_finish{false};

        auto tasks = utils::GenerateFixedArray(kTokenOwnerCount, [&](std::size_t) {
            return engine::AsyncNoSpan([&allowed_to_finish, token = wts.GetToken()] {
                while (!allowed_to_finish) {
                    // Spin.
                }
                // Release the token.
            });
        });

        // Give the tasks some time to start and enter the loop, should typically be enough in Release builds.
        std::this_thread::sleep_for(std::chrono::microseconds{5});

        allowed_to_finish = true;
        // If a race occurs between token releases, none of them will notify us, and this call will hang.
        wts.WaitForAllTokens();

        for (auto& task : tasks) {
            task.Get();
        }
    }
}

UTEST(WaitTokenStorage, AcquireTokenWhileWaiting) {
    ASSERT_EQ(GetThreadCount(), 1);

    utils::impl::WaitTokenStorage wts;

    auto task = engine::AsyncNoSpan([&, token = wts.GetToken()]() mutable {
        engine::SleepFor(10ms);

        // WaitForAllTokens is waiting for us at this point, but we need to launch
        // another task and acquire another token
        const auto another_token = wts.GetToken();
        { [[maybe_unused]] auto for_disposal = std::move(token); }

        engine::SleepFor(10ms);
    });

    wts.WaitForAllTokens();
    // This relies on the fact that there is only 1 TaskProcessor thread in this test.
    // By the time we get to run, tokens are destroyed, and task is finished as well.
    EXPECT_TRUE(task.IsFinished());
}

UTEST_MT(WaitTokenStorage, SpuriousWakeup, 3) {
    static constexpr std::size_t kTokenOwnerCount = 2;
    std::vector<engine::TaskWithResult<void>> token_owner_tasks;

    {
        utils::impl::WaitTokenStorage wts1;
        for (std::size_t i = 0; i < kTokenOwnerCount; ++i) {
            token_owner_tasks.push_back(engine::AsyncNoSpan([token = wts1.GetToken()] {}));
        }

        wts1.WaitForAllTokens();

        // It may happen that first task A does an unlock, then task B notices `IsFree() == true` and calls `Send()`
        // for the task A. So we may destroy the WTS while task A is completing `Send()`. And this should work.
    }

    // This probably reuses Impl of wts1.
    utils::impl::WaitTokenStorage wts2;

    auto token = wts2.GetToken();
    auto awaiter_task = engine::AsyncNoSpan([&wts2] { wts2.WaitForAllTokens(); });

    engine::SleepFor(10ms);
    EXPECT_FALSE(awaiter_task.IsFinished());

    token = {};
    awaiter_task.WaitFor(utest::kMaxTestWaitTime);
    EXPECT_TRUE(awaiter_task.IsFinished());

    for (auto& task : token_owner_tasks) {
        UEXPECT_NO_THROW(task.Get());
    }
    UEXPECT_NO_THROW(awaiter_task.Get());
}

TEST(WaitTokenStorage, StaticDestruction1) {
    // Imagine 'wts' is a global variable. It can only be used in the coroutine
    // context, but will be destroyed outside, after the coroutine context stops.
    utils::impl::WaitTokenStorage wts;

    engine::RunStandalone([&] {
        const auto token = wts.GetToken();
        EXPECT_EQ(wts.AliveTokensApprox(), 1);
    });

    EXPECT_EQ(wts.AliveTokensApprox(), 0);
    wts.WaitForAllTokens();
}

TEST(WaitTokenStorage, StaticDestruction2) {
    // Imagine 'wts' is a static variable. It will be constructed when its
    // enclosing function will be called inside the coroutine context, but will be
    // destroyed outside, after the coroutine context stops.
    std::optional<utils::impl::WaitTokenStorage> wts;

    engine::RunStandalone([&] {
        wts.emplace();
        const auto token = wts->GetToken();
        EXPECT_EQ(wts->AliveTokensApprox(), 1);
    });

    EXPECT_EQ(wts->AliveTokensApprox(), 0);
    wts->WaitForAllTokens();
    wts.reset();
}

USERVER_NAMESPACE_END
