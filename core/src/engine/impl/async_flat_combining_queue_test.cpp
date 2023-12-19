#include <engine/impl/async_flat_combining_queue.hpp>

#include <atomic>
#include <future>
#include <thread>

#include <compiler/relax_cpu.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/fixed_array.hpp>

using namespace std::chrono_literals;

USERVER_NAMESPACE_BEGIN

namespace {

using NodeBase = concurrent::impl::SinglyLinkedBaseHook;
using EmptyNode = concurrent::impl::SinglyLinkedBaseHook;

// If producers are consistently faster than the consumer
// (happens especially often in Debug and sanitizers builds),
// then the consumers don't have the chance to free themselves, switch places,
// and we don't fully test the queue. So we add a linearly-increasing back-off
// to test both "fast" and "slow" producers in all build modes.
class BackOff final {
 public:
  BackOff() = default;

  void operator()() noexcept {
    for (Counter i = 0; i < iterations_; ++i) {
      compiler::RelaxCpu{}();
    }
    ++iterations_;
  }

 private:
  using Counter = std::uint64_t;

  Counter iterations_{0};
};

template <typename ConsumerFunc>
void PushAndWork(engine::impl::AsyncFlatCombiningQueue& queue, NodeBase& node,
                 BackOff& back_off, ConsumerFunc consumer_func) {
  back_off();
  auto consumer = queue.PushAndTryStartConsuming(node);
  if (consumer.IsValid()) {
    std::move(consumer).ConsumeAndStop(consumer_func);
  }
}

}  // namespace

TEST(AsyncFlatCombiningQueueNoCoro, SimpleSync) {
  engine::impl::AsyncFlatCombiningQueue queue;
  EmptyNode node1;
  EmptyNode node2;

  {
    auto consumer = queue.PushAndTryStartConsuming(node1);
    ASSERT_TRUE(consumer.IsValid());
    ASSERT_EQ(consumer.TryPop(), &node1);
    ASSERT_EQ(consumer.TryPop(), nullptr);
    ASSERT_TRUE(consumer.TryStopConsuming());
  }

  {
    auto consumer = queue.PushAndTryStartConsuming(node1);
    ASSERT_TRUE(consumer.IsValid());
    ASSERT_FALSE(queue.PushAndTryStartConsuming(node2).IsValid());
    ASSERT_EQ(consumer.TryPop(), &node1);
    ASSERT_FALSE(consumer.TryStopConsuming());
    ASSERT_EQ(consumer.TryPop(), &node2);
    ASSERT_TRUE(consumer.TryStopConsuming());
    ASSERT_FALSE(consumer.IsValid());
  }
}

UTEST(AsyncFlatCombiningQueue, SimpleAsync) {
  engine::impl::AsyncFlatCombiningQueue queue;
  EmptyNode node1;
  EmptyNode node2;
  EmptyNode node3;

  // This task will run after the parent starts waiting (single-threaded TP).
  auto task = engine::AsyncNoSpan([&] {
    engine::Yield();
    ASSERT_FALSE(queue.PushAndTryStartConsuming(node1).IsValid());
    ASSERT_FALSE(queue.PushAndTryStartConsuming(node2).IsValid());
    ASSERT_FALSE(queue.PushAndTryStartConsuming(node3).IsValid());
  });

  // No cancellation support in AsyncFlatCombiningQueue.
  const engine::TaskCancellationBlocker cancel_blocker;

  auto consumer = queue.WaitAndStartConsuming();
  ASSERT_TRUE(consumer.IsValid());
  ASSERT_EQ(consumer.TryPop(), nullptr);

  // Here we sleep, and 'task' runs until the end
  queue.WaitWhileEmpty(consumer);
  ASSERT_EQ(consumer.TryPop(), &node1);
  ASSERT_EQ(consumer.TryPop(), &node2);
  // Should return immediately.
  queue.WaitWhileEmpty(consumer);
  ASSERT_EQ(consumer.TryPop(), &node3);
  ASSERT_EQ(consumer.TryPop(), nullptr);
  ASSERT_TRUE(consumer.TryStopConsuming());

  ASSERT_NO_THROW(task.Get());
}

TEST(AsyncFlatCombiningQueueNoCoro, StressSync) {
  constexpr std::size_t kThreadCount = 3;

  std::uint64_t nodes_consumed = 0;
  std::atomic<bool> keep_running{true};
  engine::impl::AsyncFlatCombiningQueue queue;

  auto threads = utils::GenerateFixedArray(kThreadCount, [&](std::size_t) {
    return std::async([&] {
      BackOff back_off;

      // NOLINTBEGIN(clang-analyzer-cplusplus.NewDeleteLeaks)
      while (keep_running) {
        PushAndWork(queue, *new EmptyNode{}, back_off,
                    [&](NodeBase& node) noexcept {
                      ++nodes_consumed;
                      delete &node;
                    });
      }
      // NOLINTEND(clang-analyzer-cplusplus.NewDeleteLeaks)
    });
  });

  std::this_thread::sleep_for(100ms);
  keep_running = false;

  for (auto& thread : threads) {
    EXPECT_NO_THROW(thread.get());
  }
  EXPECT_GE(nodes_consumed, 10);
}

UTEST_MT(AsyncFlatCombiningQueue, StressAsync, 3) {
  const auto producers_count = GetThreadCount() - 1;
  const auto test_deadline = engine::Deadline::FromDuration(100ms);
  constexpr auto kTimeForModeChange = 1ms;

  std::uint64_t nodes_consumed_sync = 0;
  std::uint64_t nodes_consumed_async = 0;
  std::uint64_t mode_changed_iterations = 0;
  engine::impl::AsyncFlatCombiningQueue queue;

  auto tasks = utils::GenerateFixedArray(producers_count, [&](std::size_t) {
    return engine::AsyncNoSpan([&] {
      BackOff back_off;

      // NOLINTBEGIN(clang-analyzer-cplusplus.NewDeleteLeaks)
      while (!engine::current_task::ShouldCancel()) {
        PushAndWork(queue, *new EmptyNode{}, back_off,
                    [&](NodeBase& node) noexcept {
                      ++nodes_consumed_sync;
                      delete &node;
                    });
      }
      // NOLINTEND(clang-analyzer-cplusplus.NewDeleteLeaks)
    });
  });

  while (!test_deadline.IsReached()) {
    // No cancellation support in AsyncFlatCombiningQueue.
    const engine::TaskCancellationBlocker cancel_blocker;

    auto consumer = queue.WaitAndStartConsuming();
    ASSERT_TRUE(consumer.IsValid());
    const auto mode_change_deadline =
        engine::Deadline::FromDuration(kTimeForModeChange);

    const auto consumer_func = [&](NodeBase& node) noexcept {
      ++nodes_consumed_async;
      delete &node;
    };

    while (!mode_change_deadline.IsReached()) {
      queue.WaitWhileEmpty(consumer);
      while (auto* const node = consumer.TryPop()) {
        consumer_func(*node);
      }
    }

    std::move(consumer).ConsumeAndStop(consumer_func);
    ++mode_changed_iterations;

    engine::SleepFor(kTimeForModeChange);
  }

  for (auto& task : tasks) {
    task.RequestCancel();
  }

  for (auto& task : tasks) {
    EXPECT_NO_THROW(task.Get());
  }
  EXPECT_GE(nodes_consumed_sync, 10);
  EXPECT_GE(nodes_consumed_async, 10);
  EXPECT_GE(mode_changed_iterations, 2);
}

USERVER_NAMESPACE_END
