#include <userver/utest/utest.hpp>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <future>
#include <thread>
#include <vector>

#include <userver/formats/json/value_builder.hpp>
#include <userver/utils/rand.hpp>
#include <userver/utils/statistics/min_max_avg.hpp>

USERVER_NAMESPACE_BEGIN

constexpr size_t kStressNumThreads = 2;
constexpr auto kStressTestDuration = std::chrono::milliseconds{500};

static_assert(
    utils::statistics::kHasWriterSupport<utils::statistics::MinMaxAvg<int>>);

template <int... values>
auto GetFilledMma() {
  utils::statistics::MinMaxAvg<int> mma;
  (mma.Account(values), ...);
  return mma;
}

void CheckCurrent(const utils::statistics::MinMaxAvg<int>& mma,
                  int expected_min, int expected_max, int expected_avg) {
  const auto current = mma.GetCurrent();
  EXPECT_EQ(expected_min, current.minimum);
  EXPECT_EQ(expected_max, current.maximum);
  EXPECT_EQ(expected_avg, current.average);
}

TEST(MinMaxAvg, Default) {
  utils::statistics::MinMaxAvg<int> mma;
  CheckCurrent(mma, 0, 0, 0);
}

TEST(MinMaxAvg, Simple) {
  auto mma = GetFilledMma<-2, 1, 2, 7>();
  CheckCurrent(mma, -2, 7, 2);
}

TEST(MinMaxAvg, Add) {
  auto mma = GetFilledMma<1, 2>();
  mma.Add(GetFilledMma<3, 0, 4>());
  CheckCurrent(mma, 0, 4, 2);
}

TEST(MinMaxAvg, Reset) {
  auto mma = GetFilledMma<1>();
  CheckCurrent(mma, 1, 1, 1);
  mma.Reset();
  CheckCurrent(mma, 0, 0, 0);
}

TEST(MinMaxAvg, NegativeInt64Average) {
  utils::statistics::MinMaxAvg<int64_t> mma;
  mma.Account(-30);
  mma.Account(-10);
  EXPECT_EQ(-20, mma.GetCurrent().average);
}

TEST(MinMaxAvg, FloatingAverage) {
  utils::statistics::MinMaxAvg<int, double> mma;
  mma.Account(1);
  mma.Account(2);
  EXPECT_EQ(1.5, mma.GetCurrent().average);
}

TEST(MinMaxAvg, ToJson) {
  auto value =
      formats::json::ValueBuilder(GetFilledMma<1, 2, 3>()).ExtractValue();
  ASSERT_TRUE(value.IsObject());
  EXPECT_EQ(3, value.GetSize());
  EXPECT_EQ(1, value["min"].As<int>(-1));
  EXPECT_EQ(3, value["max"].As<int>(-1));
  EXPECT_EQ(2, value["avg"].As<int>(-1));
}

TEST(MinMaxAvg, FloatingAverageToJson) {
  utils::statistics::MinMaxAvg<int, double> mma;
  mma.Account(1);
  mma.Account(2);

  const auto value = formats::json::ValueBuilder(mma).ExtractValue();
  EXPECT_EQ(1.5, value["avg"].As<double>(-1));
}

TEST(MinMaxAvg, Stress) {
  using MmaType = utils::statistics::MinMaxAvg<int64_t>;

  std::atomic<bool> is_running{true};
  MmaType shared;
  const auto work = [&] {
    MmaType local;
    while (is_running) {
      const auto x = utils::Rand();
      local.Account(x);
      shared.Account(x);
    }
    return local;
  };

  std::vector<std::future<MmaType>> futures;
  for (size_t i = 0; i < kStressNumThreads; ++i)
    futures.push_back(std::async(std::launch::async, work));

  std::this_thread::sleep_for(kStressTestDuration);
  is_running = false;

  MmaType sum_locals;
  for (auto& f : futures) {
    sum_locals.Add(f.get());
  }

  const auto sum_locals_final = sum_locals.GetCurrent();
  const auto shared_final = shared.GetCurrent();
  EXPECT_EQ(sum_locals_final.minimum, shared_final.minimum);
  EXPECT_EQ(sum_locals_final.maximum, shared_final.maximum);
  EXPECT_EQ(sum_locals_final.average, shared_final.average);
}

USERVER_NAMESPACE_END
