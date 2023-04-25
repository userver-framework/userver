#include <userver/concurrent/lazy_value.hpp>

#include <functional>

#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/async.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::tests {

UTEST(LazyValues, Types) {
  EXPECT_EQ(concurrent::LazyValue<int>([]() { return 2; })(), 2);
  int x{};
  EXPECT_EQ(concurrent::LazyValue<int>([&x]() { return 2 + x; })(), 2);
  EXPECT_EQ(
      concurrent::LazyValue<int>(std::function<int()>([]() { return 2; }))(),
      2);
}

UTEST(LazyValues, SingleCoroutine) {
  int count = 0;
  concurrent::LazyValue<int> value([&count] {
    count++;
    return 1;
  });

  EXPECT_EQ(count, 0);
  EXPECT_EQ(value(), 1);
  EXPECT_EQ(count, 1);
  EXPECT_EQ(value(), 1);
  EXPECT_EQ(count, 1);
}

UTEST(LazyValues, MutableLambda) {
  int count = 0;
  concurrent::LazyValue<int> value([&count]() mutable {
    count++;
    return 1;
  });

  EXPECT_EQ(count, 0);
  EXPECT_EQ(value(), 1);
  EXPECT_EQ(count, 1);
  EXPECT_EQ(value(), 1);
  EXPECT_EQ(count, 1);
}

UTEST(LazyValue, Exception) {
  int count = 0;
  concurrent::LazyValue<int> value([&count] {
    count++;
    throw std::runtime_error("error");
    return 1;
  });

  EXPECT_EQ(count, 0);
  UEXPECT_THROW(value(), std::runtime_error);
  EXPECT_EQ(count, 1);
  UEXPECT_THROW(value(), std::runtime_error);
  EXPECT_EQ(count, 1);
}

UTEST(LazyValue, Cancellation) {
  engine::SingleConsumerEvent event;
  auto task = utils::Async("test", [&] {
    int count = 0;
    concurrent::LazyValue<int> value([&] {
      EXPECT_TRUE(event.WaitForEvent());
      count++;
      engine::current_task::CancellationPoint();
      return 1;
    });

    EXPECT_EQ(count, 0);
    value();
    FAIL() << "Not thrown";
  });

  task.RequestCancel();
  event.Send();
  UEXPECT_THROW(task.Get(), engine::TaskCancelledException);
}

UTEST(LazyValue, Parallel) {
  int count = 0;
  concurrent::LazyValue<int> value([&count] {
    engine::SleepFor(std::chrono::milliseconds(100));
    count++;
    return 1;
  });

  std::vector<engine::TaskWithResult<int>> tasks;
  tasks.reserve(5);
  for (int i = 0; i < 5; i++) {
    tasks.push_back(utils::Async("test", [&value]() { return value(); }));
  }

  for (auto& task : tasks) {
    EXPECT_EQ(task.Get(), 1);
  }
  EXPECT_EQ(count, 1);
}

}  // namespace utils::tests

USERVER_NAMESPACE_END
