#include <userver/engine/impl/blocking_future.hpp>

#include <thread>

#include <userver/engine/wait_any.hpp>

#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

using namespace std::chrono_literals;

constexpr int kExpectedValue = 42;

UTEST(BlockingFuture, Basic) {
  engine::impl::BlockingPromise<int> promise;
  auto future = promise.get_future();
  promise.set_value(kExpectedValue);
  EXPECT_EQ(future.get(), kExpectedValue);
}

UTEST(BlockingFuture, NoFuture) {
  engine::impl::BlockingPromise<int> promise;
  promise.set_value(kExpectedValue);
}

UTEST(BlockingFuture, NoValue) {
  engine::impl::BlockingPromise<int> promise;
  auto future = promise.get_future();
}

UTEST(BlockingFuture, WaitAnyDeadPromise) {
  engine::impl::BlockingFuture<int> future;
  {
    engine::impl::BlockingPromise<int> promise;
    future = promise.get_future();
  }

  auto opt_index = engine::WaitAnyFor(utest::kMaxTestWaitTime, future);
  ASSERT_TRUE(opt_index);
  ASSERT_EQ(*opt_index, 0);

  UEXPECT_THROW(future.get(), std::exception);
}

UTEST(BlockingFuture, WaitAnyPromiseSetInOtherThread) {
  engine::impl::BlockingPromise<int> promise;
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

UTEST(BlockingFuture, WaitAnyPromiseSetInOtherThread2) {
  engine::impl::BlockingPromise<int> promise;
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

UTEST(BlockingFuture, WaitAnyPromiseKilledInOtherThread) {
  engine::impl::BlockingPromise<int> promise;
  auto future = promise.get_future();

  std::thread th(
      [promise = std::move(promise)]() { std::this_thread::sleep_for(100ms); });

  auto opt_index = engine::WaitAnyFor(utest::kMaxTestWaitTime, future);

  th.join();

  ASSERT_TRUE(opt_index);
  ASSERT_EQ(*opt_index, 0);

  UEXPECT_THROW(future.get(), std::exception);
}

USERVER_NAMESPACE_END
