#include <userver/rcu/read_indicator.hpp>

#include <userver/engine/mutex.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/utils/async.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {
constexpr std::size_t kReadersCount = 3;
constexpr std::size_t kCheckersCount = 1;
}  // namespace

UTEST_MT(ReadIndicator, TortureTest, kReadersCount + kCheckersCount) {
  rcu::ReadIndicator indicator;

  std::atomic<bool> keep_running{true};
  std::vector<engine::TaskWithResult<void>> tasks;

  rcu::ReadIndicatorLock indicator_lock = indicator.Lock();
  engine::Mutex ping_pong_mutex;

  for (std::size_t i = 0; i < kReadersCount; ++i) {
    tasks.push_back(utils::Async("ping-pong", [&] {
      while (keep_running.load(std::memory_order_acquire)) {
        std::lock_guard lock(ping_pong_mutex);
        indicator_lock = rcu::ReadIndicatorLock(indicator_lock);
      }
    }));
  }

  for (std::size_t i = 0; i < kCheckersCount; ++i) {
    tasks.push_back(utils::Async("checker", [&] {
      while (keep_running.load(std::memory_order_acquire)) {
        ASSERT_FALSE(indicator.IsFree());
      }
    }));
  }

  engine::SleepFor(std::chrono::milliseconds{100});
  keep_running = false;
  for (auto& task : tasks) {
    task.Get();
  }

  indicator_lock = rcu::ReadIndicatorLock{};
  EXPECT_TRUE(indicator.IsFree());
}

USERVER_NAMESPACE_END
