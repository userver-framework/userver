#include <gtest/gtest.h>

#include <engine/task/task_processor.hpp>

#include <userver/engine/run_standalone.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class Registry final {
 public:
  Registry() {
    engine::RegisterThreadStartedHook([this]() { counter_.fetch_add(1); });
  }

  void ResetCounter() { counter_.store(0); }

  size_t GetCounter() { return counter_.load(); }

 private:
  std::atomic<std::size_t> counter_{0};
};

Registry registry;

}  // namespace

TEST(ThreadStartedHook, Simple) {
  constexpr std::size_t kWorkerThreads = 20;

  registry.ResetCounter();

  engine::RunStandalone(kWorkerThreads, [] {});

  EXPECT_EQ(registry.GetCounter(), kWorkerThreads);
}

USERVER_NAMESPACE_END
