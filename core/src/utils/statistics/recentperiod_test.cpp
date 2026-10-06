#include <userver/utils/statistics/rate_counter.hpp>
#include <userver/utils/statistics/recentperiod.hpp>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

namespace {
class TestTimer {
public:
    using duration = std::chrono::system_clock::duration;

    static std::chrono::system_clock::time_point now() { return std::chrono::system_clock::time_point(timer); }

    static void sleep(duration duration) { timer += duration; }

    static void reset() { timer = duration{0}; }

private:
    static duration timer;
};

TestTimer::duration TestTimer::timer{0};

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
    using Duration = typename utils::statistics::RecentPeriod<Atomic, Result>::Duration;
    const std::atomic<Duration> duration{Duration{}};
    EXPECT_TRUE(duration.is_lock_free());
}

TEST(RecentPeriod, Basic) {
    utils::statistics::RecentPeriod<Atomic, Result, TestTimer> stat(std::chrono::seconds(10), std::chrono::seconds(60));

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
    utils::statistics::RecentPeriod<int, int> stat(std::chrono::seconds(60), std::chrono::seconds(60));

    EXPECT_EQ(0, stat.GetStatsForPeriod());
    EXPECT_EQ(0, stat.GetStatsForPeriod(std::chrono::seconds{60}, true));

    stat.GetCurrentCounter() += 1;
    EXPECT_EQ(0, stat.GetStatsForPeriod());
    EXPECT_EQ(1, stat.GetStatsForPeriod(std::chrono::seconds{60}, true));
}

namespace custom {

struct AdlCounter {
    unsigned long value{0};
};

void ResetMetric(AdlCounter& counter) { counter.value = 0; }

}  // namespace custom

struct AdlResult {
    unsigned long counter{0};

    AdlResult& operator+=(const custom::AdlCounter& value) {
        counter += value.value;
        return *this;
    }
};

TEST(RecentPeriod, AdlResetMetric) {
    utils::statistics::RecentPeriod<custom::AdlCounter, AdlResult, TestTimer>
        stat(std::chrono::seconds(10), std::chrono::seconds(60));

    stat.GetCurrentCounter().value += 5;
    EXPECT_EQ(stat.GetStatsForPeriod(std::chrono::seconds{60}, true).counter, 5U);

    stat.Reset();
    EXPECT_EQ(stat.GetStatsForPeriod(std::chrono::seconds{60}, true).counter, 0U);
}

struct RateCounterResult {
    utils::statistics::Rate value{};

    RateCounterResult& operator+=(const utils::statistics::RateCounter& counter) {
        value += counter.Load();
        return *this;
    }
};

TEST(RecentPeriod, RateCounterResetMetric) {
    utils::statistics::RecentPeriod<utils::statistics::RateCounter, RateCounterResult, TestTimer>
        stat(std::chrono::seconds(10), std::chrono::seconds(60));

    stat.GetCurrentCounter() += utils::statistics::Rate{7};
    EXPECT_EQ(stat.GetStatsForPeriod(std::chrono::seconds{60}, true).value, utils::statistics::Rate{7});

    stat.Reset();
    EXPECT_EQ(stat.GetStatsForPeriod(std::chrono::seconds{60}, true).value, utils::statistics::Rate{0});
}

USERVER_NAMESPACE_END
