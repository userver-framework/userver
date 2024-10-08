#include <userver/utest/utest.hpp>

#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>

#include <userver/utils/mock_now.hpp>
#include <userver/utils/statistics/busy.hpp>

USERVER_NAMESPACE_BEGIN

namespace {
std::chrono::system_clock::time_point tp_start(std::chrono::seconds(1));
}

using utils::datetime::MockNowSet;
using utils::datetime::MockSleep;

TEST(Busy, Busy100) {
  MockNowSet(tp_start);

  utils::statistics::BusyStorage storage(std::chrono::seconds(1),
                                         std::chrono::seconds(5));

  EXPECT_FLOAT_EQ(0, storage.GetCurrentLoad());

  storage.StartWork();
  MockSleep(std::chrono::seconds(10));
  EXPECT_FLOAT_EQ(1, storage.GetCurrentLoad());
  storage.StopWork();

  EXPECT_FLOAT_EQ(1, storage.GetCurrentLoad());
}

TEST(Busy, Sample) {
  MockNowSet(tp_start);

  auto some_heavy_task_that_takes_1s = []() {
    MockSleep(std::chrono::seconds(1));
    return true;
  };

  auto some_heavy_task_that_takes_3s = []() {
    MockSleep(std::chrono::seconds(3));
  };

  auto sleep_for_more_than_5s = []() { MockSleep(std::chrono::seconds(10)); };

  using namespace std::literals::chrono_literals;
  /// [busy sample]
  utils::statistics::BusyStorage storage(1s, 5s);
  EXPECT_FLOAT_EQ(0, storage.GetCurrentLoad());

  {
    utils::statistics::BusyMarker scope{storage};
    if (some_heavy_task_that_takes_1s()) {
      some_heavy_task_that_takes_3s();
      // 4 seconds out of 5 were consumed -> 80%
      EXPECT_FLOAT_EQ(0.8, storage.GetCurrentLoad());
    }
  }
  EXPECT_LE(0.2, storage.GetCurrentLoad());

  sleep_for_more_than_5s();
  // No workload measured in last 5 seconds
  EXPECT_FLOAT_EQ(0, storage.GetCurrentLoad());
  /// [busy sample]

  {
    utils::statistics::BusyMarker scope0{storage};
    std::optional<utils::statistics::BusyMarker> scope1{storage};
    utils::statistics::BusyMarker scope2{storage};
    std::optional<utils::statistics::BusyMarker> scope3{storage};
    utils::statistics::BusyMarker scope4{storage};
    utils::statistics::BusyMarker scope5{storage};
    some_heavy_task_that_takes_1s();
    scope3.reset();
    scope1.reset();
    EXPECT_LE(0.2, storage.GetCurrentLoad());
  }
  EXPECT_LE(0.2, storage.GetCurrentLoad());
}

TEST(Busy, Busy100Recursive) {
  MockNowSet(tp_start);

  utils::statistics::BusyStorage storage(std::chrono::seconds(1),
                                         std::chrono::seconds(5));

  EXPECT_FLOAT_EQ(0, storage.GetCurrentLoad());

  storage.StartWork();
  storage.StartWork();
  MockSleep(std::chrono::seconds(10));
  EXPECT_FLOAT_EQ(1, storage.GetCurrentLoad());
  storage.StopWork();

  EXPECT_FLOAT_EQ(1, storage.GetCurrentLoad());
  storage.StopWork();

  EXPECT_FLOAT_EQ(1, storage.GetCurrentLoad());
}

TEST(Busy, Busy100Cycle) {
  MockNowSet(tp_start);

  utils::statistics::BusyStorage storage(std::chrono::seconds(1),
                                         std::chrono::seconds(5));

  EXPECT_FLOAT_EQ(0, storage.GetCurrentLoad());

  for (int i = 0; i < 100; i++) {
    storage.StartWork();
    MockSleep(std::chrono::milliseconds(200));
    storage.StopWork();
  }

  EXPECT_FLOAT_EQ(1, storage.GetCurrentLoad());
}

TEST(Busy, Single) {
  MockNowSet(tp_start);

  utils::statistics::BusyStorage storage(std::chrono::seconds(1),
                                         std::chrono::seconds(5));

  EXPECT_FLOAT_EQ(0, storage.GetCurrentLoad());

  storage.StartWork();
  EXPECT_FLOAT_EQ(0, storage.GetCurrentLoad());

  MockSleep(std::chrono::seconds(1));
  EXPECT_FLOAT_EQ(0.2, storage.GetCurrentLoad());

  storage.StopWork();
  EXPECT_FLOAT_EQ(0.2, storage.GetCurrentLoad());

  for (size_t i = 0; i < 5; i++) {
    MockSleep(std::chrono::seconds(1));
    EXPECT_FLOAT_EQ(0.2, storage.GetCurrentLoad());
  }

  MockSleep(std::chrono::seconds(1));
  EXPECT_FLOAT_EQ(0, storage.GetCurrentLoad());
}

TEST(Busy, TooBusy) {
  MockNowSet(tp_start);

  utils::statistics::BusyStorage storage(std::chrono::seconds(1),
                                         std::chrono::seconds(5));

  storage.StartWork();
  EXPECT_GE(1, storage.GetCurrentLoad());

  for (int i = 0; i < 10; i++) {
    MockSleep(std::chrono::seconds(1));
    EXPECT_GE(1, storage.GetCurrentLoad());
  }

  for (int i = 0; i < 10; i++) {
    MockSleep(std::chrono::seconds(1));
    EXPECT_FLOAT_EQ(1, storage.GetCurrentLoad());
  }

  EXPECT_FLOAT_EQ(1, storage.GetCurrentLoad());

  MockSleep(std::chrono::milliseconds(100));
  storage.StopWork();
  EXPECT_FLOAT_EQ(1, storage.GetCurrentLoad());

  for (int i = 0; i < 5; i++) {
    MockSleep(std::chrono::seconds(1));
    EXPECT_LT(0, storage.GetCurrentLoad());
  }

  MockSleep(std::chrono::seconds(1));
  EXPECT_FLOAT_EQ(0, storage.GetCurrentLoad());
}

USERVER_NAMESPACE_END
