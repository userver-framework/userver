#include <utest/utest.hpp>

#include <atomic>

#include <userver/engine/async.hpp>
#include <userver/engine/condition_variable.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/cancel.hpp>

namespace {

struct CountGuard {
  CountGuard(std::atomic<int>& count) : count_(count) { count_++; }
  CountGuard(CountGuard&& other) : count_(other.count_) { count_++; }
  CountGuard(const CountGuard& other) : count_(other.count_) { count_++; }

  ~CountGuard() { count_--; }

 private:
  std::atomic<int>& count_;
};

struct OverloadsTest {
  std::size_t overload1{0};
  std::size_t overload2{0};
  std::size_t overload3{0};

  mutable std::size_t overload4{0};
  mutable std::size_t overload5{0};
  mutable std::size_t overload6{0};

  static std::size_t overload7;
  static std::size_t overload8;
  static std::size_t overload9;

  void operator()(std::string&, std::string&&) & { ++overload1; }
  void operator()(const std::string&, std::string&&) & { ++overload2; }
  void operator()(const std::string&, const std::string&) & { ++overload3; }

  void operator()(std::string&, std::string&&) const& { ++overload4; }
  void operator()(const std::string&, std::string&&) const& { ++overload5; }
  void operator()(const std::string&, const std::string&) const& {
    ++overload6;
  }

  void operator()(std::string&, std::string&&) && { ++overload7; }
  void operator()(const std::string&, std::string&&) && { ++overload8; }
  void operator()(const std::string&, const std::string&) && { ++overload9; }
};

std::size_t OverloadsTest::overload7{0};
std::size_t OverloadsTest::overload8{0};
std::size_t OverloadsTest::overload9{0};

struct CountingConstructions {
  static int constructions;
  static int destructions;

  CountingConstructions() { ++constructions; }
  CountingConstructions(CountingConstructions&&) noexcept { ++constructions; }
  CountingConstructions(const CountingConstructions&) { ++constructions; }

  void operator()() const { /*noop*/
  }

  CountingConstructions& operator=(CountingConstructions&&) = default;
  CountingConstructions& operator=(const CountingConstructions&) = default;

  ~CountingConstructions() { ++destructions; }
};

int CountingConstructions::constructions = 0;
int CountingConstructions::destructions = 0;

}  // namespace

UTEST(Task, ArgumentsLifetime) {
  std::atomic<int> count = 0;

  EXPECT_EQ(0, count.load());
  auto task = engine::impl::Async([](CountGuard) {}, CountGuard(count));
  EXPECT_EQ(1, count.load());

  engine::Yield();
  EXPECT_EQ(0, count.load());

  task.Get();
  EXPECT_EQ(0, count.load());
}

UTEST(Task, ArgumentsLifetimeThrow) {
  std::atomic<int> count = 0;

  EXPECT_EQ(0, count.load());
  auto task = engine::impl::Async(
      [](CountGuard) { throw std::runtime_error("123"); }, CountGuard(count));
  EXPECT_EQ(1, count.load());

  engine::Yield();
  EXPECT_EQ(0, count.load());

  EXPECT_THROW(task.Get(), std::runtime_error);
  EXPECT_EQ(0, count.load());
}

UTEST(Task, FunctionLifetime) {
  std::atomic<int> count = 0;

  EXPECT_EQ(0, count.load());
  auto task = engine::impl::Async([guard = CountGuard(count)] {});
  EXPECT_EQ(1, count.load());

  engine::Yield();
  EXPECT_EQ(0, count.load());

  task.Get();
  EXPECT_EQ(0, count.load());
}

UTEST(Task, FunctionLifetimeThrow) {
  std::atomic<int> count = 0;

  EXPECT_EQ(0, count.load());
  auto task = engine::impl::Async(
      [guard = CountGuard(count)] { throw std::runtime_error("123"); });
  EXPECT_EQ(1, count.load());

  engine::Yield();
  EXPECT_EQ(0, count.load());

  EXPECT_THROW(task.Get(), std::runtime_error);
  EXPECT_EQ(0, count.load());
}

UTEST(Async, OverloadSelection) {
  OverloadsTest tst;
  const std::string const_string{};
  std::string nonconst_string{};

  engine::impl::Async(std::ref(tst), nonconst_string, std::string{}).Wait();
  EXPECT_EQ(tst.overload1, 1);

  engine::impl::Async(std::ref(tst), const_string, std::string{}).Wait();
  EXPECT_EQ(tst.overload2, 1);

  engine::impl::Async(std::ref(tst), const_string, const_string).Wait();
  EXPECT_EQ(tst.overload3, 1);

  engine::impl::Async(std::cref(tst), nonconst_string, std::string{}).Wait();
  EXPECT_EQ(tst.overload4, 1);

  engine::impl::Async(std::cref(tst), const_string, std::string{}).Wait();
  EXPECT_EQ(tst.overload5, 1);

  engine::impl::Async(std::cref(tst), const_string, const_string).Wait();
  EXPECT_EQ(tst.overload6, 1);

  engine::impl::Async(std::move(tst), nonconst_string, std::string{}).Wait();
  EXPECT_EQ(tst.overload7, 1);

  engine::impl::Async(std::move(tst), const_string, std::string{}).Wait();
  EXPECT_EQ(tst.overload8, 1);

  engine::impl::Async(std::move(tst), const_string, const_string).Wait();
  EXPECT_EQ(tst.overload9, 1);

  auto* function_ptr = +[]() {};
  static_assert(std::is_pointer<decltype(function_ptr)>::value, "");
  EXPECT_NO_THROW(engine::impl::Async(function_ptr).Wait());
}

UTEST(Async, ResourceDeallocation) {
  engine::impl::Async(CountingConstructions{}).Wait();
  EXPECT_EQ(CountingConstructions::constructions,
            CountingConstructions::destructions);
}

const auto kDeadlineTestsTimeout = std::chrono::milliseconds(100);
const auto kMaxTestTimeout = std::chrono::milliseconds(10000);
const auto kMaxTestDuration = std::chrono::milliseconds(5000);

TEST(Task, CurrentTaskSetDeadline) {
  RunInCoro([] {
    auto start = std::chrono::steady_clock::now();
    auto task = engine::impl::Async([] {
      engine::current_task::SetDeadline(
          engine::Deadline::FromDuration(kMaxTestTimeout));
      engine::InterruptibleSleepFor(std::chrono::milliseconds(2));
      EXPECT_FALSE(engine::current_task::IsCancelRequested());

      engine::current_task::SetDeadline(
          engine::Deadline::FromDuration(kDeadlineTestsTimeout));
      engine::InterruptibleSleepFor(kMaxTestTimeout);
      EXPECT_TRUE(engine::current_task::IsCancelRequested());
    });

    EXPECT_NO_THROW(task.Get());
    auto finish = std::chrono::steady_clock::now();
    auto duration = finish - start;
    EXPECT_GE(duration, kDeadlineTestsTimeout);
    EXPECT_LT(duration, kMaxTestDuration);
  });
}

TEST(Async, WithDeadline) {
  RunInCoro([] {
    auto start = std::chrono::steady_clock::now();
    std::atomic<bool> started{false};
    auto task = engine::impl::Async(
        engine::Deadline::FromDuration(kDeadlineTestsTimeout), [&started] {
          started = true;
          EXPECT_FALSE(engine::current_task::IsCancelRequested());
          engine::InterruptibleSleepFor(kMaxTestTimeout);
          EXPECT_TRUE(engine::current_task::IsCancelRequested());
        });

    EXPECT_NO_THROW(task.Get());
    EXPECT_TRUE(started.load());
    auto finish = std::chrono::steady_clock::now();
    auto duration = finish - start;
    EXPECT_GE(duration, kDeadlineTestsTimeout);
    EXPECT_LT(duration, kMaxTestDuration);
  });
}

TEST(Async, WithDeadlineDetach) {
  RunInCoro([] {
    std::atomic<bool> started{false};
    std::atomic<bool> finished{false};
    auto task = engine::impl::Async([&started, &finished] {
      auto start = std::chrono::steady_clock::now();
      engine::impl::Async(
          engine::Deadline::FromDuration(kDeadlineTestsTimeout),
          [start, &started, &finished] {
            started = true;
            EXPECT_FALSE(engine::current_task::IsCancelRequested());
            engine::InterruptibleSleepFor(kMaxTestTimeout);
            EXPECT_TRUE(engine::current_task::IsCancelRequested());

            auto finish = std::chrono::steady_clock::now();
            auto duration = finish - start;
            EXPECT_GE(duration, kDeadlineTestsTimeout);
            EXPECT_LT(duration, kMaxTestDuration);
            finished = true;
          })
          .Detach();
    });
    EXPECT_NO_THROW(task.Get());
    EXPECT_TRUE(started.load());
    while (!finished) engine::Yield();
  });
}
