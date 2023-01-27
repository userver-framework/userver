#include <benchmark/benchmark.h>

#include <future>
#include <vector>

#include <moodycamel/concurrentqueue.h>
#include <boost/lockfree/queue.hpp>

#include <concurrent/impl/intrusive_mpsc_queue.hpp>
#include <userver/utils/fixed_array.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

struct Node final : public concurrent::impl::SinglyLinkedBaseHook {
  int foo{};
};

class BoostLockfree final {
 public:
  explicit BoostLockfree(std::size_t /*producer_count*/) {}

  void Produce(std::size_t /*producer_id*/) { queue_.push(0); }

  bool TryConsume() {
    int result{};
    const bool success = queue_.pop(result);
    if (success) {
      benchmark::DoNotOptimize(result);
    }
    return success;
  }

 private:
  boost::lockfree::queue<int> queue_{0};
};

class MoodycamelImplicit final {
 public:
  explicit MoodycamelImplicit(std::size_t /*producer_count*/) {}

  void Produce(std::size_t /*producer_id*/) { queue_.enqueue(0); }

  bool TryConsume() {
    int result{};
    const bool success = queue_.try_dequeue(result);
    if (success) {
      benchmark::DoNotOptimize(result);
    }
    return success;
  }

 private:
  moodycamel::ConcurrentQueue<int> queue_{0};
};

class MoodycamelExplicit final {
 public:
  explicit MoodycamelExplicit(std::size_t producer_count)
      : queue_(0, producer_count, 0),
        producer_tokens_(producer_count, queue_),
        consumer_token_(queue_) {}

  void Produce(std::size_t producer_id) {
    queue_.enqueue(producer_tokens_[producer_id], 0);
  }

  bool TryConsume() {
    int result{};
    const bool success = queue_.try_dequeue(consumer_token_, result);
    if (success) {
      benchmark::DoNotOptimize(result);
    }
    return success;
  }

 private:
  moodycamel::ConcurrentQueue<int> queue_;
  utils::FixedArray<moodycamel::ProducerToken> producer_tokens_;
  moodycamel::ConsumerToken consumer_token_;
};

class IntrusiveMpscQueue final {
 public:
  explicit IntrusiveMpscQueue(std::size_t /*producer_count*/) {}

  ~IntrusiveMpscQueue() {
    while (auto* const node = queue_.TryPop()) {
      delete node;
    }
  }

  void Produce(std::size_t /*producer_id*/) { queue_.Push(*new Node()); }

  bool TryConsume() {
    auto* const node = queue_.TryPop();
    if (node) {
      benchmark::DoNotOptimize(node->foo);
      delete node;
      return true;
    } else {
      return false;
    }
  }

 private:
  concurrent::impl::IntrusiveMpscQueue<Node> queue_;
};

}  // namespace

template <typename Queue>
void MpscQueueProduce(benchmark::State& state) {
  Queue queue(state.range(0));
  std::atomic<bool> keep_running{true};
  std::vector<std::future<void>> producers;
  producers.reserve(state.range(0) - 1);

  for (int i = 0; i < state.range(0) - 1; ++i) {
    producers.push_back(std::async([&, producer_id = i + 1] {
      while (keep_running) {
        queue.Produce(producer_id);
      }
    }));
  }

  auto consumer = std::async([&] {
    while (keep_running) {
      if (!queue.TryConsume()) {
        __builtin_ia32_pause();
      }
    }
  });

  for (auto _ : state) {
    queue.Produce(0);
  }

  keep_running = false;
  for (auto& producer : producers) producer.get();
  consumer.get();
}

BENCHMARK_TEMPLATE(MpscQueueProduce, BoostLockfree)
    ->RangeMultiplier(2)
    ->Range(1, 32);
BENCHMARK_TEMPLATE(MpscQueueProduce, MoodycamelImplicit)
    ->RangeMultiplier(2)
    ->Range(1, 32);
BENCHMARK_TEMPLATE(MpscQueueProduce, MoodycamelExplicit)
    ->RangeMultiplier(2)
    ->Range(1, 32);
BENCHMARK_TEMPLATE(MpscQueueProduce, IntrusiveMpscQueue)
    ->RangeMultiplier(2)
    ->Range(1, 32);

template <typename Queue>
void MpscQueueConsume(benchmark::State& state) {
  Queue queue(state.range(0));
  std::atomic<bool> keep_running{true};
  std::vector<std::future<void>> producers;
  producers.reserve(state.range(0));

  for (int i = 0; i < state.range(0); ++i) {
    producers.push_back(std::async([&, producer_id = i] {
      while (keep_running) {
        queue.Produce(producer_id);
      }
    }));
  }

  for (auto _ : state) {
    while (!queue.TryConsume()) {
      // No 'pause', keep hammering until we receive an item.
    }
  }

  keep_running = false;
  for (auto& producer : producers) producer.get();
}

BENCHMARK_TEMPLATE(MpscQueueConsume, BoostLockfree)
    ->RangeMultiplier(2)
    ->Range(1, 32);
BENCHMARK_TEMPLATE(MpscQueueConsume, MoodycamelImplicit)
    ->RangeMultiplier(2)
    ->Range(1, 32);
BENCHMARK_TEMPLATE(MpscQueueConsume, MoodycamelExplicit)
    ->RangeMultiplier(2)
    ->Range(1, 32);
BENCHMARK_TEMPLATE(MpscQueueConsume, IntrusiveMpscQueue)
    ->RangeMultiplier(2)
    ->Range(1, 32);

template <typename Queue>
void MpscQueueProduceConsume(benchmark::State& state) {
  static constexpr std::size_t kNodeCount = 128;

  Queue queue1(1);
  Queue queue2(1);
  std::atomic<bool> keep_running{true};

  for (std::size_t i = 0; i < kNodeCount; ++i) {
    queue1.Produce(0);
  }

  auto another_worker = std::async([&] {
    while (keep_running) {
      while (!queue1.TryConsume()) {
      }
      queue2.Produce(0);
    }
  });

  for (auto _ : state) {
    while (!queue2.TryConsume()) {
    }
    queue1.Produce(0);
  }

  keep_running = false;
  // To unblock 'another_worker'
  queue1.Produce(0);
  another_worker.get();
}

BENCHMARK_TEMPLATE(MpscQueueProduceConsume, BoostLockfree);
BENCHMARK_TEMPLATE(MpscQueueProduceConsume, MoodycamelImplicit);
BENCHMARK_TEMPLATE(MpscQueueProduceConsume, MoodycamelExplicit);
BENCHMARK_TEMPLATE(MpscQueueProduceConsume, IntrusiveMpscQueue);

void IntrusiveMpscQueueProduceConsumeNoAlloc(benchmark::State& state) {
  static constexpr std::size_t kNodeCount = 128;
  using Queue = concurrent::impl::IntrusiveMpscQueue<Node>;

  utils::FixedArray<Node> nodes(kNodeCount);
  Queue queue1;
  Queue queue2;
  for (auto& node : nodes) {
    queue1.Push(node);
  }

  std::atomic<bool> keep_running{true};

  auto another_worker = std::async([&] {
    while (keep_running) {
      Node* node = nullptr;
      while (!node) {
        node = queue1.TryPop();
      }
      queue2.Push(*node);
    }
  });

  for (auto _ : state) {
    Node* node = nullptr;
    while (!node) {
      node = queue2.TryPop();
    }
    queue1.Push(*node);
  }

  keep_running = false;

  Node extra_node;
  // To unblock 'another_worker'
  queue1.Push(extra_node);
  another_worker.get();
}

BENCHMARK(IntrusiveMpscQueueProduceConsumeNoAlloc);

template <typename Queue>
void MpscQueueProduceConsumeNoContention(benchmark::State& state) {
  Queue queue(1);

  for (auto _ : state) {
    queue.Produce(0);
    queue.TryConsume();
  }
}

BENCHMARK_TEMPLATE(MpscQueueProduceConsumeNoContention, BoostLockfree);
BENCHMARK_TEMPLATE(MpscQueueProduceConsumeNoContention, MoodycamelImplicit);
BENCHMARK_TEMPLATE(MpscQueueProduceConsumeNoContention, MoodycamelExplicit);
BENCHMARK_TEMPLATE(MpscQueueProduceConsumeNoContention, IntrusiveMpscQueue);

void IntrusiveMpscQueueProduceConsumeNoContentionNoAlloc(
    benchmark::State& state) {
  Node node;
  concurrent::impl::IntrusiveMpscQueue<Node> queue;

  for (auto _ : state) {
    queue.Push(node);
    benchmark::DoNotOptimize(queue.TryPop());
  }
}

BENCHMARK(IntrusiveMpscQueueProduceConsumeNoContentionNoAlloc);

USERVER_NAMESPACE_END
