#include <userver/utest/utest.hpp>

#include <chrono>
#include <exception>
#include <future>
#include <string>
#include <thread>
#include <vector>

#include <userver/engine/async.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/exception.hpp>
#include <userver/engine/future.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/engine/wait_any.hpp>
#include <userver/utils/async.hpp>

using namespace std::chrono_literals;

USERVER_NAMESPACE_BEGIN

namespace {

constexpr std::chrono::milliseconds kWaitPeriod{10};

auto GetTimePoint() { return std::chrono::steady_clock::now() + kWaitPeriod; }

auto GetDeadline() { return engine::Deadline::FromDuration(kWaitPeriod); }

class MyException : public std::exception {
 public:
  static auto Create() { return std::make_exception_ptr(MyException{}); }
};

template <typename T>
class Future : public ::testing::Test {};

using TypesToTest = ::testing::Types<void, int, std::string>;

template <typename T>
void Ignore(T&&) {}

}  // namespace

TYPED_UTEST_SUITE(Future, TypesToTest);

TYPED_UTEST(Future, Empty) {
  engine::Future<TypeParam> f;
  EXPECT_FALSE(f.valid());
  UEXPECT_THROW(f.get(), std::future_error);
  UEXPECT_THROW(Ignore(f.wait()), std::future_error);
  UEXPECT_THROW(f.wait_for(kWaitPeriod), std::future_error);
  UEXPECT_THROW(f.wait_until(GetTimePoint()), std::future_error);
  UEXPECT_THROW(f.wait_until(GetDeadline()), std::future_error);
}

UTEST(Future, ValueVoid) {
  engine::Promise<void> p;
  auto f = p.get_future();
  auto task = engine::AsyncNoSpan([&p] { p.set_value(); });
  UEXPECT_NO_THROW(f.get());
  UEXPECT_THROW(f.get(), std::future_error);
  task.Get();
}

UTEST(Future, ValueInt) {
  engine::Promise<int> p;
  auto f = p.get_future();
  auto task = engine::AsyncNoSpan([&p] { p.set_value(42); });
  EXPECT_EQ(42, f.get());
  UEXPECT_THROW(f.get(), std::future_error);
  task.Get();
}

UTEST(Future, ValueString) {
  engine::Promise<std::string> p;
  auto f = p.get_future();
  auto task = engine::AsyncNoSpan([&p] { p.set_value("test"); });
  EXPECT_EQ("test", f.get());
  UEXPECT_THROW(f.get(), std::future_error);
  task.Get();
}

TYPED_UTEST(Future, Exception) {
  engine::Promise<TypeParam> p;
  auto f = p.get_future();
  auto task =
      engine::AsyncNoSpan([&p] { p.set_exception(MyException::Create()); });
  UEXPECT_THROW(f.get(), MyException);
  UEXPECT_THROW(f.get(), std::future_error);
  task.Get();
}

TYPED_UTEST(Future, FutureAlreadyRetrieved) {
  engine::Promise<TypeParam> p;
  UEXPECT_NO_THROW(Ignore(p.get_future()));
  UEXPECT_THROW(Ignore(p.get_future()), std::future_error);
}

UTEST(Future, AlreadySatisfiedValueVoid) {
  engine::Promise<void> p;
  UEXPECT_NO_THROW(p.set_value());
  UEXPECT_THROW(p.set_value(), std::future_error);
  UEXPECT_THROW(p.set_exception(MyException::Create()), std::future_error);
  UEXPECT_NO_THROW(p.get_future().get());
  UEXPECT_THROW(p.set_value(), std::future_error);
  UEXPECT_THROW(p.set_exception(MyException::Create()), std::future_error);
}

UTEST(Future, AlreadySatisfiedValueInt) {
  engine::Promise<int> p;
  UEXPECT_NO_THROW(p.set_value(42));
  UEXPECT_THROW(p.set_value(13), std::future_error);
  UEXPECT_THROW(p.set_exception(MyException::Create()), std::future_error);
  UEXPECT_NO_THROW(p.get_future().get());
  UEXPECT_THROW(p.set_value(-1), std::future_error);
  UEXPECT_THROW(p.set_exception(MyException::Create()), std::future_error);
}

UTEST(Future, AlreadySatisfiedValueString) {
  engine::Promise<std::string> p;
  UEXPECT_NO_THROW(p.set_value("test"));
  UEXPECT_THROW(p.set_value("bad"), std::future_error);
  UEXPECT_THROW(p.set_exception(MyException::Create()), std::future_error);
  UEXPECT_NO_THROW(p.get_future().get());
  UEXPECT_THROW(p.set_value("set"), std::future_error);
  UEXPECT_THROW(p.set_exception(MyException::Create()), std::future_error);
}

UTEST(Future, AlreadySatisfiedExceptionVoid) {
  engine::Promise<void> p;
  UEXPECT_NO_THROW(p.set_exception(MyException::Create()));
  UEXPECT_THROW(p.set_value(), std::future_error);
  UEXPECT_THROW(p.set_exception(MyException::Create()), std::future_error);
  UEXPECT_THROW(p.get_future().get(), MyException);
  UEXPECT_THROW(p.set_value(), std::future_error);
  UEXPECT_THROW(p.set_exception(MyException::Create()), std::future_error);
}

UTEST(Future, AlreadySatisfiedExceptionInt) {
  engine::Promise<int> p;
  UEXPECT_NO_THROW(p.set_exception(MyException::Create()));
  UEXPECT_THROW(p.set_value(13), std::future_error);
  UEXPECT_THROW(p.set_exception(MyException::Create()), std::future_error);
  UEXPECT_THROW(p.get_future().get(), MyException);
  UEXPECT_THROW(p.set_value(-1), std::future_error);
  UEXPECT_THROW(p.set_exception(MyException::Create()), std::future_error);
}

UTEST(Future, AlreadySatisfiedExceptionString) {
  engine::Promise<std::string> p;
  UEXPECT_NO_THROW(p.set_exception(MyException::Create()));
  UEXPECT_THROW(p.set_value("bad"), std::future_error);
  UEXPECT_THROW(p.set_exception(MyException::Create()), std::future_error);
  UEXPECT_THROW(p.get_future().get(), MyException);
  UEXPECT_THROW(p.set_value("set"), std::future_error);
  UEXPECT_THROW(p.set_exception(MyException::Create()), std::future_error);
}

TYPED_UTEST(Future, BrokenPromise) {
  auto f = engine::Promise<TypeParam>{}.get_future();
  UEXPECT_THROW(f.get(), std::future_error);
}

UTEST(Future, WaitVoid) {
  engine::Promise<void> p;
  auto f = p.get_future();
  auto task = engine::AsyncNoSpan([&p] { p.set_value(); });
  EXPECT_EQ(engine::FutureStatus::kReady, f.wait());
  EXPECT_EQ(engine::FutureStatus::kReady, f.wait());
  task.Get();
}

UTEST(Future, WaitInt) {
  engine::Promise<int> p;
  auto f = p.get_future();
  auto task = engine::AsyncNoSpan([&p] { p.set_value(42); });
  EXPECT_EQ(engine::FutureStatus::kReady, f.wait());
  EXPECT_EQ(engine::FutureStatus::kReady, f.wait());
  task.Get();
}

UTEST(Future, WaitString) {
  engine::Promise<std::string> p;
  auto f = p.get_future();
  auto task = engine::AsyncNoSpan([&p] { p.set_value("test"); });
  EXPECT_EQ(engine::FutureStatus::kReady, f.wait());
  EXPECT_EQ(engine::FutureStatus::kReady, f.wait());
  task.Get();
}

TYPED_UTEST(Future, Timeout) {
  engine::Promise<TypeParam> p;
  auto f = p.get_future();
  EXPECT_EQ(engine::FutureStatus::kTimeout, f.wait_for(kWaitPeriod));
  EXPECT_EQ(engine::FutureStatus::kTimeout, f.wait_until(GetTimePoint()));
  EXPECT_EQ(engine::FutureStatus::kTimeout, f.wait_until(GetDeadline()));
}

TYPED_UTEST(Future, Cancel) {
  engine::SingleConsumerEvent started_event;
  engine::SingleConsumerEvent cancelled_event;
  engine::Promise<TypeParam> p;
  auto task = engine::AsyncNoSpan(
      [&](engine::Future<TypeParam> f) {
        started_event.Send();
        ASSERT_FALSE(cancelled_event.WaitForEventFor(utest::kMaxTestWaitTime));
        EXPECT_EQ(engine::FutureStatus::kCancelled, f.wait());
        EXPECT_EQ(engine::FutureStatus::kCancelled, f.wait_for(kWaitPeriod));
        EXPECT_EQ(engine::FutureStatus::kCancelled,
                  f.wait_until(GetTimePoint()));
        EXPECT_EQ(engine::FutureStatus::kCancelled,
                  f.wait_until(GetDeadline()));
        UEXPECT_THROW(f.get(), engine::WaitInterruptedException);

        engine::TaskCancellationBlocker block_cancel;
        EXPECT_EQ(engine::FutureStatus::kTimeout, f.wait_for(kWaitPeriod));
        EXPECT_EQ(engine::FutureStatus::kTimeout, f.wait_until(GetTimePoint()));
        EXPECT_EQ(engine::FutureStatus::kTimeout, f.wait_until(GetDeadline()));
      },
      p.get_future());
  ASSERT_TRUE(started_event.WaitForEventFor(utest::kMaxTestWaitTime));
  task.SyncCancel();
}

UTEST_MT(Future, ThreadedGetSet, 2) {
  static constexpr auto kTestDuration = 200ms;
  static constexpr size_t kAttempts = 100;

  const auto test_deadline = engine::Deadline::FromDuration(kTestDuration);
  while (!test_deadline.IsReached()) {
    std::vector<engine::Promise<int>> promises(kAttempts);
    std::vector<engine::Future<int>> futures;
    futures.reserve(kAttempts);
    for (auto& promise : promises) {
      futures.push_back(promise.get_future());
    }

    auto task = engine::AsyncNoSpan([&promises] {
      for (auto& promise : promises) {
        promise.set_value(42);
      }
    });

    for (auto& future : futures) {
      ASSERT_EQ(future.get(), 42);
    }
    task.Get();
  }
}

UTEST(Future, SampleFuture) {
  /// [Sample engine::Future usage]
  engine::Promise<int> int_promise;
  engine::Promise<std::string> string_promise;
  auto deadline = GetDeadline();

  constexpr auto kBadValue = -1;
  constexpr auto kFallbackString = "Bad string";

  constexpr auto kTestValue = 777;
  constexpr auto kTestString = "Test string value";

  auto int_future = int_promise.get_future();
  auto string_future = string_promise.get_future();

  auto calc_task =
      utils::Async("calc", [int_promise = std::move(int_promise),
                            string_promise = std::move(string_promise),
                            kTestValue]() mutable {
        // Emulating long calculation of x and s...
        engine::SleepFor(100ms);
        int_promise.set_value(kTestValue);

        engine::SleepFor(100ms);
        string_promise.set_value(kTestString);
        // Other calculations.
      });

  auto int_consumer = utils::Async(
      "int_consumer",
      [deadline, int_future = std::move(int_future), kTestValue]() mutable {
        auto status = int_future.wait_until(deadline);
        auto x = kBadValue;
        switch (status) {
          case engine::FutureStatus::kReady:
            x = int_future.get();
            ASSERT_EQ(x, kTestValue);
            // ...
            break;
          case engine::FutureStatus::kTimeout:
            // Timeout Handling
            break;
          case engine::FutureStatus::kCancelled:
            // Handling cancellation of calculations
            // (example, return to queue)
            return;
        }
      });

  auto string_consumer = utils::Async(
      "string_consumer",
      [string_future = std::move(string_future), kTestString]() mutable {
        std::string s;
        try {
          s = string_future.get();
          ASSERT_EQ(s, kTestString);
        } catch (const std::exception& ex) {
          // Exception Handling
          s = kFallbackString;
        }
        // ...
      });

  calc_task.Get();
  int_consumer.Get();
  string_consumer.Get();
  /// [Sample engine::Future usage]
}

UTEST(Future, WaitAnyDeadPromise) {
  engine::Future<int> future;
  {
    engine::Promise<int> promise;
    future = promise.get_future();
  }

  auto opt_index = engine::WaitAnyFor(utest::kMaxTestWaitTime, future);
  ASSERT_TRUE(opt_index);
  ASSERT_EQ(*opt_index, 0);

  UEXPECT_THROW(future.get(), std::exception);
}

UTEST(Future, WaitAnyPromiseSetInOtherThread) {
  static constexpr int kExpectedValue = 42;

  engine::Promise<int> promise;
  auto future = promise.get_future();

  std::thread th([promise = std::move(promise)]() mutable {
    std::this_thread::sleep_for(100ms);
    promise.set_value(kExpectedValue);
  });

  auto opt_index = engine::WaitAnyFor(utest::kMaxTestWaitTime, future);

  th.join();

  ASSERT_TRUE(opt_index);
  ASSERT_EQ(*opt_index, 0);

  EXPECT_EQ(future.get(), kExpectedValue);
}

UTEST(Future, WaitAnyPromiseSetInOtherThread2) {
  static constexpr int kExpectedValue = 42;

  engine::Promise<int> promise;
  auto future = promise.get_future();

  std::thread th([&promise]() {
    std::this_thread::sleep_for(100ms);
    promise.set_value(kExpectedValue);
  });

  auto opt_index = engine::WaitAnyFor(utest::kMaxTestWaitTime, future);

  th.join();

  ASSERT_TRUE(opt_index);
  ASSERT_EQ(*opt_index, 0);

  EXPECT_EQ(future.get(), kExpectedValue);
}

UTEST(Future, WaitAnyPromiseKilledInOtherThread) {
  engine::Promise<int> promise;
  auto future = promise.get_future();

  std::thread th(
      [promise = std::move(promise)]() { std::this_thread::sleep_for(100ms); });

  auto opt_index = engine::WaitAnyFor(utest::kMaxTestWaitTime, future);

  th.join();

  ASSERT_TRUE(opt_index);
  ASSERT_EQ(*opt_index, 0);

  UEXPECT_THROW(future.get(), std::future_error);
}

USERVER_NAMESPACE_END
