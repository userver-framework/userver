#include <userver/utils/impl/wait_token_storage.hpp>

#include <atomic>
#include <stdexcept>
#include <string>
#include <vector>

#include <userver/engine/async.hpp>
#include <userver/engine/run_standalone.hpp>
#include <userver/engine/shared_mutex.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/utest/utest.hpp>

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
    utils::impl::WaitTokenStorage wts;

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
  for (std::size_t i = 0; i < kLauncherCount; ++i) {
    launcher_tasks.push_back(engine::AsyncNoSpan([&] {
      // Give all the launcher tasks time to start before the TaskProcessor is
      // clobbered by the detached tasks
      engine::SleepFor(1ms);

      for (std::size_t j = 0; j < kWorkersPerLauncher; ++j) {
        // Note: the token is created in one task and moved into another one
        engine::AsyncNoSpan([&, token = wts.GetToken()] {
          std::shared_lock allowed_to_finish_lock(allowed_to_finish);
          ++workers_completed;
        }).Detach();
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

UTEST(WaitTokenStorage, AcquireTokenWhileWaiting) {
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
  EXPECT_TRUE(task.IsFinished());
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
