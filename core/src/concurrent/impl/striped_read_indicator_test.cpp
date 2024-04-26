#include <userver/concurrent/impl/striped_read_indicator.hpp>

#include <thread>

#include <userver/engine/async.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {
constexpr std::size_t kReadersCount = 3;
constexpr std::size_t kCheckersCount = 1;
}  // namespace

UTEST_MT(StripedReadIndicator, LockPassingStress,
         kReadersCount + kCheckersCount) {
  concurrent::impl::StripedReadIndicator indicator;
  concurrent::impl::StripedReadIndicatorLock indicator_lock = indicator.Lock();
  engine::Mutex ping_pong_mutex;

  std::atomic<bool> keep_running{true};
  std::vector<engine::TaskWithResult<void>> tasks;

  for (std::size_t i = 0; i < kReadersCount; ++i) {
    tasks.push_back(engine::AsyncNoSpan([&] {
      while (keep_running) {
        const std::lock_guard ping_pong_mutex_lock{ping_pong_mutex};
        auto lock_copy = indicator_lock;
        indicator_lock = std::move(lock_copy);
        // Give other reader threads a chance to lock ping_pong_mutex.
        std::this_thread::yield();
      }
    }));
  }

  for (std::size_t i = 0; i < kCheckersCount; ++i) {
    tasks.push_back(engine::AsyncNoSpan([&] {
      while (keep_running) {
        ASSERT_FALSE(indicator.IsFree());
      }
    }));
  }

  engine::SleepFor(std::chrono::milliseconds{100});
  keep_running = false;
  for (auto& task : tasks) {
    task.Get();
  }

  indicator_lock = concurrent::impl::StripedReadIndicatorLock{};
  EXPECT_TRUE(indicator.IsFree());
}

UTEST(StripedReadIndicator, Metrics) {
  concurrent::impl::StripedReadIndicator indicator;

  EXPECT_TRUE(indicator.IsFree());
  EXPECT_EQ(indicator.GetAcquireCountApprox(), 0);
  EXPECT_EQ(indicator.GetReleaseCountApprox(), 0);
  EXPECT_EQ(indicator.GetActiveCountApprox(), 0);
  {
    const auto lock1 = indicator.Lock();
    EXPECT_FALSE(indicator.IsFree());
    EXPECT_EQ(indicator.GetAcquireCountApprox(), 1);
    EXPECT_EQ(indicator.GetReleaseCountApprox(), 0);
    EXPECT_EQ(indicator.GetActiveCountApprox(), 1);
    {
      const auto lock2 = indicator.Lock();
      EXPECT_FALSE(indicator.IsFree());
      EXPECT_EQ(indicator.GetAcquireCountApprox(), 2);
      EXPECT_EQ(indicator.GetReleaseCountApprox(), 0);
      EXPECT_EQ(indicator.GetActiveCountApprox(), 2);
    }
    EXPECT_FALSE(indicator.IsFree());
    EXPECT_EQ(indicator.GetAcquireCountApprox(), 2);
    EXPECT_EQ(indicator.GetReleaseCountApprox(), 1);
    EXPECT_EQ(indicator.GetActiveCountApprox(), 1);
  }
  EXPECT_TRUE(indicator.IsFree());
  EXPECT_EQ(indicator.GetAcquireCountApprox(), 2);
  EXPECT_EQ(indicator.GetReleaseCountApprox(), 2);
  EXPECT_EQ(indicator.GetActiveCountApprox(), 0);
}

USERVER_NAMESPACE_END
