#include <utest/utest.hpp>

#include <atomic>

#include <engine/async.hpp>
#include <engine/sleep.hpp>

namespace {
struct Guard {
  Guard(std::atomic<int>& count) : count_(count) { count_++; }
  Guard(Guard&& other) : count_(other.count_) { count_++; }
  Guard(const Guard& other) : count_(other.count_) { count_++; }

  ~Guard() { count_--; }

 private:
  std::atomic<int>& count_;
};
}  // namespace

TEST(Task, ArgumentsLifetime) {
  RunInCoro(
      [] {
        std::atomic<int> count = 0;

        EXPECT_EQ(0, count.load());
        auto task = engine::impl::Async([](Guard) {}, Guard(count));
        EXPECT_EQ(1, count.load());

        engine::Yield();
        EXPECT_EQ(0, count.load());

        task.Get();
        EXPECT_EQ(0, count.load());
      },
      1);
}

TEST(Task, ArgumentsLifetimeThrow) {
  RunInCoro(
      [] {
        std::atomic<int> count = 0;

        EXPECT_EQ(0, count.load());
        auto task = engine::impl::Async(
            [](Guard) { throw std::runtime_error("123"); }, Guard(count));
        EXPECT_EQ(1, count.load());

        engine::Yield();
        EXPECT_EQ(0, count.load());

        EXPECT_THROW(task.Get(), std::runtime_error);
        EXPECT_EQ(0, count.load());
      },
      1);
}

TEST(Task, FunctionLifetime) {
  RunInCoro(
      [] {
        std::atomic<int> count = 0;

        EXPECT_EQ(0, count.load());
        auto task = engine::impl::Async([guard = Guard(count)] {});
        EXPECT_EQ(1, count.load());

        engine::Yield();
        EXPECT_EQ(0, count.load());

        task.Get();
        EXPECT_EQ(0, count.load());
      },
      1);
}

TEST(Task, FunctionLifetimeThrow) {
  RunInCoro(
      [] {
        std::atomic<int> count = 0;

        EXPECT_EQ(0, count.load());
        auto task = engine::impl::Async(
            [guard = Guard(count)] { throw std::runtime_error("123"); });
        EXPECT_EQ(1, count.load());

        engine::Yield();
        EXPECT_EQ(0, count.load());

        EXPECT_THROW(task.Get(), std::runtime_error);
        EXPECT_EQ(0, count.load());
      },
      1);
}
