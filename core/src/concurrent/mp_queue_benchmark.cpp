#include <benchmark/benchmark.h>

#include <userver/concurrent/mpsc_queue.hpp>
#include <userver/concurrent/queue.hpp>
#include <userver/engine/run_standalone.hpp>
#include <userver/utils/async.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

template <typename QueueType>
auto GetProducerTask(std::shared_ptr<QueueType> queue, std::atomic<bool>& run) {
  return utils::Async("producer", [producer = queue->GetProducer(), &run] {
    std::size_t message = 0;
    while (run) {
      bool res = producer.Push(std::size_t{message++});
      benchmark::DoNotOptimize(res);
    }
  });
}

template <typename QueueType>
auto GetConsumerTask(std::shared_ptr<QueueType> queue,
                     const std::atomic<bool>& run) {
  return utils::Async("consumer", [consumer = queue->GetConsumer(), &run]() {
    std::size_t value{};
    while (run) {
      bool res = consumer.Pop(value);
      benchmark::DoNotOptimize(res);
    }
  });
}
}  // namespace

template <typename QueueType>
void producer_consumer(benchmark::State& state) {
  engine::RunStandalone(state.range(0) + state.range(1), [&] {
    std::size_t ProducersCount = state.range(0);
    std::size_t ConsumersCount = state.range(1);
    std::size_t QueueSize = state.range(2);

    std::atomic<bool> run{true};
    auto queue = QueueType::Create(QueueSize);

    std::vector<engine::TaskWithResult<void>> tasks;
    tasks.reserve(ProducersCount + ConsumersCount - 1);
    for (std::size_t i = 0; i < ProducersCount - 1; ++i) {
      tasks.push_back(GetProducerTask(queue, run));
    }

    for (std::size_t i = 0; i < ConsumersCount; ++i) {
      tasks.push_back(GetConsumerTask(queue, run));
    }

    // Current thread work
    {
      std::size_t message = 0;
      auto producer = queue->GetProducer();
      for (auto _ : state) {
        bool res = producer.Push(std::size_t{message++});
        benchmark::DoNotOptimize(res);
      }
    }

    run = false;
  });
}

BENCHMARK_TEMPLATE(producer_consumer, concurrent::NonFifoMpmcQueue<std::size_t>)
    ->RangeMultiplier(2)
    ->Ranges({{1, 4}, {1, 4}, {128, 512}});

BENCHMARK_TEMPLATE(producer_consumer, concurrent::NonFifoMpmcQueue<std::size_t>)
    ->RangeMultiplier(2)
    ->Ranges({{1, 4}, {1, 4}, {1'000'000'000, 1'000'000'000}});

BENCHMARK_TEMPLATE(producer_consumer, concurrent::NonFifoMpscQueue<std::size_t>)
    ->RangeMultiplier(2)
    ->Ranges({{1, 4}, {1, 1}, {1'000'000'000, 1'000'000'000}});

BENCHMARK_TEMPLATE(producer_consumer, concurrent::SpmcQueue<std::size_t>)
    ->RangeMultiplier(2)
    ->Ranges({{1, 1}, {1, 4}, {1'000'000'000, 1'000'000'000}});

BENCHMARK_TEMPLATE(producer_consumer, concurrent::SpscQueue<std::size_t>)
    ->RangeMultiplier(2)
    ->Ranges({{1, 1}, {1, 1}, {1'000'000'000, 1'000'000'000}});

BENCHMARK_TEMPLATE(producer_consumer, concurrent::MpscQueue<std::size_t>)
    ->RangeMultiplier(2)
    ->Ranges({{1, 4}, {1, 1}, {128, 512}});

BENCHMARK_TEMPLATE(producer_consumer, concurrent::MpscQueue<std::size_t>)
    ->RangeMultiplier(2)
    ->Ranges({{1, 4}, {1, 1}, {1'000'000'000, 1'000'000'000}});

USERVER_NAMESPACE_END
