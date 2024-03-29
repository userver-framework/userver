#include <userver/concurrent/striped_counter.hpp>

#include <atomic>
#include <limits>

#include <userver/engine/async.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/fixed_array.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

using StripedCounter = concurrent::StripedCounter;

constexpr std::size_t kThreads = 32;

}  // namespace

TEST(StripedCounter, Works) {
  StripedCounter counter{};
  counter.Add(2);
  EXPECT_EQ(2, counter.Read());
  EXPECT_EQ(2, counter.NonNegativeRead());
}

TEST(StripedCounter, IncrementDecrement) {
  StripedCounter counter{};
  counter.Add(5);
  EXPECT_EQ(5, counter.Read());
  EXPECT_EQ(5, counter.NonNegativeRead());
  counter.Subtract(5);
  EXPECT_EQ(0, counter.Read());
  EXPECT_EQ(0, counter.NonNegativeRead());

  counter.Subtract(1);
  EXPECT_EQ(std::numeric_limits<std::uintptr_t>::max(), counter.Read());
  EXPECT_EQ(0, counter.NonNegativeRead());
}

UTEST_MT(StripedCounter, Stress, kThreads + 1) {
  const auto test_deadline =
      engine::Deadline::FromDuration(std::chrono::milliseconds{100});
  std::atomic<bool> keep_running{true};
  StripedCounter counter{};

  auto tasks = utils::GenerateFixedArray(kThreads, [&](std::size_t) {
    return engine::AsyncNoSpan([&] {
      std::uint64_t local_counter = 0;
      while (keep_running.load(std::memory_order_acquire)) {
        counter.Add(1);
        ++local_counter;
      }
      return local_counter;
    });
  });

  engine::SleepUntil(test_deadline);
  keep_running = false;
  std::uint64_t total_count = 0;
  for (auto& task : tasks) {
    UEXPECT_NO_THROW(total_count += task.Get());
  }

  EXPECT_EQ(counter.Read(), total_count);
}

USERVER_NAMESPACE_END
