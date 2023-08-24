#include <userver/utils/statistics/recentperiod.hpp>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

namespace {
class TestTimer {
 public:
  using duration = std::chrono::system_clock::duration;

  static std::chrono::system_clock::time_point now() {
    return std::chrono::system_clock::time_point(timer_);
  }

  static void sleep(duration duration) { timer_ += duration; }

  static void reset() { timer_ = duration{0}; }

 private:
  static duration timer_;
};

TestTimer::duration TestTimer::timer_{0};

}  // namespace

struct Atomic {
  std::atomic_ulong counter;

  void Reset() { counter = 0; }
};

struct Result {
  unsigned long counter{0};

  using Duration = std::chrono::steady_clock::duration;

  Result& Add(const Atomic& a, Duration, Duration) {
    counter += a.counter.load();
    return *this;
  }
};

TEST(RecentPeriod, TimerIsAtomic) {
  using Duration =
      typename utils::statistics::RecentPeriod<Atomic, Result>::Duration;
  std::atomic<Duration> duration;
  EXPECT_TRUE(duration.is_lock_free());
}

TEST(RecentPeriod, Basic) {
  utils::statistics::RecentPeriod<Atomic, Result, TestTimer> stat(
      std::chrono::seconds(10), std::chrono::seconds(60));

  for (int i = 1; i < 10; i++) {
    stat.GetCurrentCounter().counter += i;
    TestTimer::sleep(std::chrono::seconds(10));
  }

  {
    auto result = stat.GetStatsForPeriod();
    EXPECT_EQ(result.counter, 39U);
  }

  {
    TestTimer::sleep(std::chrono::seconds(10));
    auto result = stat.GetStatsForPeriod();
    EXPECT_EQ(result.counter, 35U);
  }

  {
    TestTimer::sleep(std::chrono::seconds(60));
    auto result = stat.GetStatsForPeriod();
    EXPECT_EQ(result.counter, 0U);
  }
}

TEST(RecentPeriod, IntegralResult) {
  utils::statistics::RecentPeriod<int, int> stat(std::chrono::seconds(60),
                                                 std::chrono::seconds(60));

  EXPECT_EQ(0, stat.GetStatsForPeriod());
  EXPECT_EQ(0, stat.GetStatsForPeriod(std::chrono::seconds{60}, true));

  stat.GetCurrentCounter() += 1;
  EXPECT_EQ(0, stat.GetStatsForPeriod());
  EXPECT_EQ(1, stat.GetStatsForPeriod(std::chrono::seconds{60}, true));
}

USERVER_NAMESPACE_END
