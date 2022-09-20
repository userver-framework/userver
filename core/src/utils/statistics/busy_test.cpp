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

  auto id = storage.StartWork();
  MockSleep(std::chrono::seconds(10));
  EXPECT_FLOAT_EQ(1, storage.GetCurrentLoad());
  storage.StopWork(id);

  EXPECT_FLOAT_EQ(1, storage.GetCurrentLoad());
}

TEST(Busy, Busy100Cycle) {
  MockNowSet(tp_start);

  utils::statistics::BusyStorage storage(std::chrono::seconds(1),
                                         std::chrono::seconds(5));

  EXPECT_FLOAT_EQ(0, storage.GetCurrentLoad());

  for (int i = 0; i < 100; i++) {
    auto id = storage.StartWork();
    MockSleep(std::chrono::milliseconds(200));
    storage.StopWork(id);
  }

  EXPECT_FLOAT_EQ(1, storage.GetCurrentLoad());
}

TEST(Busy, Single) {
  MockNowSet(tp_start);

  utils::statistics::BusyStorage storage(std::chrono::seconds(1),
                                         std::chrono::seconds(5));

  EXPECT_FLOAT_EQ(0, storage.GetCurrentLoad());

  auto id = storage.StartWork();
  EXPECT_FLOAT_EQ(0, storage.GetCurrentLoad());

  MockSleep(std::chrono::seconds(1));
  EXPECT_FLOAT_EQ(0.2, storage.GetCurrentLoad());

  storage.StopWork(id);
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

  auto id = storage.StartWork();
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
  storage.StopWork(id);
  EXPECT_FLOAT_EQ(1, storage.GetCurrentLoad());

  for (int i = 0; i < 5; i++) {
    MockSleep(std::chrono::seconds(1));
    EXPECT_LT(0, storage.GetCurrentLoad());
  }

  MockSleep(std::chrono::seconds(1));
  EXPECT_FLOAT_EQ(0, storage.GetCurrentLoad());
}

TEST(Busy, Reentrant) {
  MockNowSet(tp_start);
  utils::statistics::BusyStorage storage(std::chrono::seconds(1),
                                         std::chrono::seconds(5), 2);
  {
    auto id1 = storage.StartWork();
    auto id2 = storage.StartWork();
    MockSleep(std::chrono::seconds(1));
    storage.StopWork(id1);
    storage.StopWork(id2);
    MockSleep(std::chrono::seconds(1));
  }
  EXPECT_FLOAT_EQ(1.0 / 10, storage.GetCurrentLoad());
}

TEST(Busy, TwoThreads) {
  MockNowSet(tp_start);
  utils::statistics::BusyStorage storage(std::chrono::seconds(1),
                                         std::chrono::seconds(5), 2);

  std::mutex m;
  std::condition_variable cv;
  bool should_stop = false;
  bool is_initialized[2] = {false, false};
  {
    auto cb = [&storage, &m, &cv, &should_stop, &is_initialized](size_t idx) {
      std::unique_lock<std::mutex> lock(m);
      utils::statistics::BusyMarker marker(storage);

      is_initialized[idx] = true;
      cv.notify_all();

      ASSERT_TRUE(cv.wait_for(lock, utest::kMaxTestWaitTime,
                              [&should_stop]() { return should_stop; }));
    };

    std::future<void> f[2];
    for (size_t i = 0; i < 2; i++) {
      f[i] = std::async(std::launch::async, [&cb, i] { return cb(i); });

      std::unique_lock<std::mutex> lock(m);
      ASSERT_TRUE(
          cv.wait_for(lock, utest::kMaxTestWaitTime,
                      [&is_initialized, i] { return is_initialized[i]; }));
    }

    MockSleep(std::chrono::seconds(1));
    {
      std::unique_lock<std::mutex> lock(m);
      should_stop = true;
      cv.notify_all();
    }
    for (auto& fut : f) fut.get();
  }
  EXPECT_FLOAT_EQ(1.0 / 5, storage.GetCurrentLoad());
}

USERVER_NAMESPACE_END
