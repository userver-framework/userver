#include <concurrent/impl/intrusive_mpsc_queue.hpp>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <future>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include <userver/utils/enumerate.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

struct Node final : public concurrent::impl::SinglyLinkedBaseHook {
  explicit Node(std::size_t x = 0) : x(x) {}

  std::size_t x{0};
};

using MpscQueue = concurrent::impl::IntrusiveMpscQueue<Node>;

constexpr std::chrono::milliseconds kStressTestDuration{200};

}  // namespace

TEST(IntrusiveMpscQueue, Empty) {
  MpscQueue queue;
  EXPECT_EQ(queue.TryPop(), nullptr);
  EXPECT_EQ(queue.TryPop(), nullptr);
  EXPECT_EQ(queue.TryPop(), nullptr);
}

TEST(IntrusiveMpscQueue, PushPopOnce) {
  MpscQueue queue;
  Node node1;

  queue.Push(node1);
  EXPECT_EQ(queue.TryPop(), &node1);
  EXPECT_EQ(queue.TryPop(), nullptr);
  EXPECT_EQ(queue.TryPop(), nullptr);
  EXPECT_EQ(queue.TryPop(), nullptr);
}

TEST(IntrusiveMpscQueue, PushPopTwice) {
  MpscQueue queue;
  Node node1;
  Node node2;

  queue.Push(node1);
  queue.Push(node2);
  EXPECT_EQ(queue.TryPop(), &node1);
  EXPECT_EQ(queue.TryPop(), &node2);
  EXPECT_EQ(queue.TryPop(), nullptr);
  EXPECT_EQ(queue.TryPop(), nullptr);
  EXPECT_EQ(queue.TryPop(), nullptr);
}

TEST(IntrusiveMpscQueue, PushPopInterleaved) {
  MpscQueue queue;
  Node node1;
  Node node2;
  Node node3;

  queue.Push(node1);
  queue.Push(node2);
  EXPECT_EQ(queue.TryPop(), &node1);

  queue.Push(node3);
  EXPECT_EQ(queue.TryPop(), &node2);
  EXPECT_EQ(queue.TryPop(), &node3);
  EXPECT_EQ(queue.TryPop(), nullptr);
  EXPECT_EQ(queue.TryPop(), nullptr);
  EXPECT_EQ(queue.TryPop(), nullptr);
}

TEST(IntrusiveMpscQueue, StressTest) {
  constexpr std::size_t kProducerCount = 2;
  constexpr std::size_t kStopSignal = -1;

  MpscQueue queue;
  std::atomic<bool> keep_producing{true};

  std::vector<std::future<std::uint64_t>> producers;
  producers.reserve(kProducerCount);

  for (std::size_t i = 0; i < kProducerCount; ++i) {
    producers.push_back(std::async([&, producer_id = i] {
      std::uint64_t produced_count = 0;
      while (keep_producing) {
        queue.Push(*new Node{producer_id});
        ++produced_count;
      }
      queue.Push(*new Node{kStopSignal});
      return produced_count;
    }));
  }

  auto consumer = std::async([&] {
    std::vector<std::uint64_t> received_counts_by_producer(kProducerCount, 0);
    std::size_t stop_signals_received = 0;

    while (true) {
      std::unique_ptr<Node> node(queue.TryPop());
      if (!node) continue;

      if (node->x == kStopSignal) {
        ++stop_signals_received;
        if (stop_signals_received == kProducerCount) {
          return received_counts_by_producer;
        }
      } else {
        ++received_counts_by_producer[node->x];
      }
    }
  });

  std::this_thread::sleep_for(kStressTestDuration);
  keep_producing = false;
  const auto received_counts_by_producer = consumer.get();

  for (const auto& [i, producer] : utils::enumerate(producers)) {
    EXPECT_EQ(received_counts_by_producer[i], producer.get());
  }
}

TEST(IntrusiveMpscQueue, StressTestNodeReuse) {
  static constexpr std::size_t kNodeCount = 4;

  Node nodes[kNodeCount];
  MpscQueue queue1;
  MpscQueue queue2;
  for (auto& node : nodes) {
    queue1.Push(node);
  }

  std::atomic<bool> keep_running{true};

  auto worker1 = std::async([&] {
    while (keep_running) {
      Node* node = queue1.TryPop();
      if (!node) continue;

      EXPECT_EQ(node->x, 0);
      queue2.Push(*node);
    }
  });

  auto worker2 = std::async([&] {
    while (keep_running) {
      Node* node = queue2.TryPop();
      if (!node) continue;

      EXPECT_EQ(node->x, 0);
      queue1.Push(*node);
    }
  });

  keep_running = false;
  worker1.get();
  worker2.get();
}

USERVER_NAMESPACE_END
