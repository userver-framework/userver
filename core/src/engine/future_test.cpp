#include <utest/utest.hpp>

#include <chrono>
#include <exception>
#include <string>
#include <vector>

#include <engine/async.hpp>
#include <engine/deadline.hpp>
#include <engine/exception.hpp>
#include <engine/future.hpp>
#include <engine/single_consumer_event.hpp>
#include <engine/task/cancel.hpp>

namespace {

static constexpr std::chrono::milliseconds kWaitPeriod{10};

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

TYPED_TEST_SUITE(Future, TypesToTest);

TYPED_TEST(Future, Empty) {
  RunInCoro([] {
    engine::Future<TypeParam> f;
    EXPECT_FALSE(f.valid());
    EXPECT_THROW(f.get(), std::future_error);
    EXPECT_THROW(Ignore(f.wait()), std::future_error);
    EXPECT_THROW(f.wait_for(kWaitPeriod), std::future_error);
    EXPECT_THROW(f.wait_until(GetTimePoint()), std::future_error);
    EXPECT_THROW(f.wait_until(GetDeadline()), std::future_error);
  });
}

TEST(Future, ValueVoid) {
  RunInCoro([] {
    engine::Promise<void> p;
    auto f = p.get_future();
    auto task = engine::impl::Async([&p] { p.set_value(); });
    EXPECT_NO_THROW(f.get());
    EXPECT_THROW(f.get(), std::future_error);
    task.Get();
  });
}

TEST(Future, ValueInt) {
  RunInCoro([] {
    engine::Promise<int> p;
    auto f = p.get_future();
    auto task = engine::impl::Async([&p] { p.set_value(42); });
    EXPECT_EQ(42, f.get());
    EXPECT_THROW(f.get(), std::future_error);
    task.Get();
  });
}

TEST(Future, ValueString) {
  RunInCoro([] {
    engine::Promise<std::string> p;
    auto f = p.get_future();
    auto task = engine::impl::Async([&p] { p.set_value("test"); });
    EXPECT_EQ("test", f.get());
    EXPECT_THROW(f.get(), std::future_error);
    task.Get();
  });
}

TYPED_TEST(Future, Exception) {
  RunInCoro([] {
    engine::Promise<TypeParam> p;
    auto f = p.get_future();
    auto task =
        engine::impl::Async([&p] { p.set_exception(MyException::Create()); });
    EXPECT_THROW(f.get(), MyException);
    EXPECT_THROW(f.get(), std::future_error);
    task.Get();
  });
}

TYPED_TEST(Future, FutureAlreadyRetrieved) {
  RunInCoro([] {
    engine::Promise<TypeParam> p;
    EXPECT_NO_THROW(Ignore(p.get_future()));
    EXPECT_THROW(Ignore(p.get_future()), std::future_error);
  });
}

TEST(Future, AlreadySatisfiedValueVoid) {
  RunInCoro([] {
    engine::Promise<void> p;
    EXPECT_NO_THROW(p.set_value());
    EXPECT_THROW(p.set_value(), std::future_error);
    EXPECT_THROW(p.set_exception(MyException::Create()), std::future_error);
    EXPECT_NO_THROW(p.get_future().get());
    EXPECT_THROW(p.set_value(), std::future_error);
    EXPECT_THROW(p.set_exception(MyException::Create()), std::future_error);
  });
}

TEST(Future, AlreadySatisfiedValueInt) {
  RunInCoro([] {
    engine::Promise<int> p;
    EXPECT_NO_THROW(p.set_value(42));
    EXPECT_THROW(p.set_value(13), std::future_error);
    EXPECT_THROW(p.set_exception(MyException::Create()), std::future_error);
    EXPECT_NO_THROW(p.get_future().get());
    EXPECT_THROW(p.set_value(-1), std::future_error);
    EXPECT_THROW(p.set_exception(MyException::Create()), std::future_error);
  });
}

TEST(Future, AlreadySatisfiedValueString) {
  RunInCoro([] {
    engine::Promise<std::string> p;
    EXPECT_NO_THROW(p.set_value("test"));
    EXPECT_THROW(p.set_value("bad"), std::future_error);
    EXPECT_THROW(p.set_exception(MyException::Create()), std::future_error);
    EXPECT_NO_THROW(p.get_future().get());
    EXPECT_THROW(p.set_value("set"), std::future_error);
    EXPECT_THROW(p.set_exception(MyException::Create()), std::future_error);
  });
}

TEST(Future, AlreadySatisfiedExceptionVoid) {
  RunInCoro([] {
    engine::Promise<void> p;
    EXPECT_NO_THROW(p.set_exception(MyException::Create()));
    EXPECT_THROW(p.set_value(), std::future_error);
    EXPECT_THROW(p.set_exception(MyException::Create()), std::future_error);
    EXPECT_THROW(p.get_future().get(), MyException);
    EXPECT_THROW(p.set_value(), std::future_error);
    EXPECT_THROW(p.set_exception(MyException::Create()), std::future_error);
  });
}

TEST(Future, AlreadySatisfiedExceptionInt) {
  RunInCoro([] {
    engine::Promise<int> p;
    EXPECT_NO_THROW(p.set_exception(MyException::Create()));
    EXPECT_THROW(p.set_value(13), std::future_error);
    EXPECT_THROW(p.set_exception(MyException::Create()), std::future_error);
    EXPECT_THROW(p.get_future().get(), MyException);
    EXPECT_THROW(p.set_value(-1), std::future_error);
    EXPECT_THROW(p.set_exception(MyException::Create()), std::future_error);
  });
}

TEST(Future, AlreadySatisfiedExceptionString) {
  RunInCoro([] {
    engine::Promise<std::string> p;
    EXPECT_NO_THROW(p.set_exception(MyException::Create()));
    EXPECT_THROW(p.set_value("bad"), std::future_error);
    EXPECT_THROW(p.set_exception(MyException::Create()), std::future_error);
    EXPECT_THROW(p.get_future().get(), MyException);
    EXPECT_THROW(p.set_value("set"), std::future_error);
    EXPECT_THROW(p.set_exception(MyException::Create()), std::future_error);
  });
}

TYPED_TEST(Future, BrokenPromise) {
  RunInCoro([] {
    auto f = engine::Promise<TypeParam>{}.get_future();
    EXPECT_THROW(f.get(), std::future_error);
  });
}

TEST(Future, WaitVoid) {
  RunInCoro([] {
    engine::Promise<void> p;
    auto f = p.get_future();
    auto task = engine::impl::Async([&p] { p.set_value(); });
    EXPECT_EQ(engine::FutureStatus::kReady, f.wait());
    EXPECT_EQ(engine::FutureStatus::kReady, f.wait());
    task.Get();
  });
}

TEST(Future, WaitInt) {
  RunInCoro([] {
    engine::Promise<int> p;
    auto f = p.get_future();
    auto task = engine::impl::Async([&p] { p.set_value(42); });
    EXPECT_EQ(engine::FutureStatus::kReady, f.wait());
    EXPECT_EQ(engine::FutureStatus::kReady, f.wait());
    task.Get();
  });
}

TEST(Future, WaitString) {
  RunInCoro([] {
    engine::Promise<std::string> p;
    auto f = p.get_future();
    auto task = engine::impl::Async([&p] { p.set_value("test"); });
    EXPECT_EQ(engine::FutureStatus::kReady, f.wait());
    EXPECT_EQ(engine::FutureStatus::kReady, f.wait());
    task.Get();
  });
}

TYPED_TEST(Future, Timeout) {
  RunInCoro([] {
    engine::Promise<TypeParam> p;
    auto f = p.get_future();
    EXPECT_EQ(engine::FutureStatus::kTimeout, f.wait_for(kWaitPeriod));
    EXPECT_EQ(engine::FutureStatus::kTimeout, f.wait_until(GetTimePoint()));
    EXPECT_EQ(engine::FutureStatus::kTimeout, f.wait_until(GetDeadline()));
  });
}

TYPED_TEST(Future, Cancel) {
  RunInCoro([] {
    engine::SingleConsumerEvent started_event;
    engine::SingleConsumerEvent cancelled_event;
    engine::Promise<TypeParam> p;
    auto task = engine::impl::Async(
        [&](engine::Future<TypeParam> f) {
          started_event.Send();
          ASSERT_FALSE(cancelled_event.WaitForEventFor(kMaxTestWaitTime));
          EXPECT_EQ(engine::FutureStatus::kCancelled, f.wait());
          EXPECT_EQ(engine::FutureStatus::kCancelled, f.wait_for(kWaitPeriod));
          EXPECT_EQ(engine::FutureStatus::kCancelled,
                    f.wait_until(GetTimePoint()));
          EXPECT_EQ(engine::FutureStatus::kCancelled,
                    f.wait_until(GetDeadline()));
          EXPECT_THROW(f.get(), engine::WaitInterruptedException);

          engine::TaskCancellationBlocker block_cancel;
          EXPECT_EQ(engine::FutureStatus::kTimeout, f.wait_for(kWaitPeriod));
          EXPECT_EQ(engine::FutureStatus::kTimeout,
                    f.wait_until(GetTimePoint()));
          EXPECT_EQ(engine::FutureStatus::kTimeout,
                    f.wait_until(GetDeadline()));
        },
        p.get_future());
    ASSERT_TRUE(started_event.WaitForEventFor(kMaxTestWaitTime));
    task.SyncCancel();
  });
}

TEST(Future, ThreadedGetSet) {
  static constexpr size_t kThreads = 2;
  static constexpr auto kTestDuration = std::chrono::milliseconds{200};
  static constexpr size_t kAttempts = 100;

  RunInCoro(
      [] {
        const auto test_deadline =
            engine::Deadline::FromDuration(kTestDuration);
        while (!test_deadline.IsReached()) {
          std::vector<engine::Promise<int>> promises(kAttempts);
          std::vector<engine::Future<int>> futures;
          futures.reserve(kAttempts);
          for (auto& promise : promises) {
            futures.push_back(promise.get_future());
          }

          auto task = engine::impl::Async([&promises] {
            for (auto& promise : promises) {
              promise.set_value(42);
            }
          });

          for (auto& future : futures) {
            ASSERT_EQ(future.get(), 42);
          }
          task.Get();
        }
      },
      kThreads);
}
