#include <userver/utest/utest.hpp>

#include <limits>

#include <concurrent/impl/striped_counter.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/run_standalone.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

using StripedCounter = concurrent::impl::StripedCounter;

}

TEST(StripedCounter, Works) {
  StripedCounter counter{};
  counter.Add(2);
  EXPECT_EQ(2, counter.Read());
}

TEST(StripedCounter, IncrementDecrement) {
  StripedCounter counter{};
  counter.Add(5);
  EXPECT_EQ(5, counter.Read());
  counter.Subtract(5);
  EXPECT_EQ(0, counter.Read());

  counter.Subtract(1);
  EXPECT_EQ(std::numeric_limits<std::uintptr_t>::max(), counter.Read());
}

TEST(StripedCounter, Concurrency) {
  constexpr std::size_t kIterations = 1'000'000;
  constexpr std::size_t kThreads = 32;
  StripedCounter counter{};
  engine::RunStandalone(kThreads, [&counter] {
    std::vector<engine::TaskWithResult<void>> tasks(kThreads);
    for (auto& task : tasks) {
      task = engine::AsyncNoSpan([&counter] {
        for (std::size_t i = 0; i < kIterations; ++i) {
          counter.Add(1);
        }
      });
    }
    for (auto& task : tasks) {
      task.Wait();
    }
  });

  EXPECT_EQ(counter.Read(), kThreads * kIterations);
}

USERVER_NAMESPACE_END
