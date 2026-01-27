#include <benchmark/benchmark.h>

#include <userver/concurrent/mpsc_queue.hpp>
#include <userver/concurrent/queue.hpp>
#include <userver/engine/run_standalone.hpp>
#include <userver/utils/async.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

template <typename Producer>
auto GetProducerTask(Producer producer, const std::atomic<bool>& run) {
    return engine::CriticalAsyncNoSpan([producer = std::move(producer), &run] {
        std::size_t message = 0;
        while (run && producer.Push(std::size_t{message++})) {
        }
    });
}

template <typename Consumer>
auto GetConsumerTask(Consumer consumer) {
    return engine::CriticalAsyncNoSpan([consumer = std::move(consumer)] {
        std::size_t value{};
        while (consumer.Pop(value)) {
            benchmark::DoNotOptimize(value);
        }
    });
}

constexpr auto kUnbounded = concurrent::NonFifoMpmcQueue<std::size_t>::kUnbounded;

}  // namespace

template <typename QueueType>
void QueueProduce(benchmark::State& state) {
    engine::RunStandalone(state.range(0) + state.range(1), [&] {
        const std::size_t producers_count = state.range(0);
        const std::size_t consumers_count = state.range(1);
        const std::size_t max_size = state.range(2);

        auto queue = QueueType::Create(max_size);
        std::atomic<bool> run{true};

        std::vector<engine::TaskWithResult<void>> producer_tasks;
        producer_tasks.reserve(producers_count - 1);
        for (std::size_t i = 0; i < producers_count - 1; ++i) {
            producer_tasks.push_back(GetProducerTask(queue->GetProducer(), run));
        }

        std::vector<engine::TaskWithResult<void>> consumer_tasks;
        consumer_tasks.reserve(consumers_count);
        for (std::size_t i = 0; i < consumers_count; ++i) {
            consumer_tasks.push_back(GetConsumerTask(queue->GetConsumer()));
        }

        {
            std::size_t message = 0;
            auto producer = queue->GetProducer();
            for ([[maybe_unused]] auto _ : state) {
                bool res = producer.Push(std::size_t{message++});
                benchmark::DoNotOptimize(res);
            }
        }

        run = false;

        for (auto& task : producer_tasks) {
            task.RequestCancel();
            task.Get();
        }
        for (auto& task : consumer_tasks) {
            task.Get();
        }
    });
}

template <typename QueueType>
void QueueConsume(benchmark::State& state) {
    engine::RunStandalone(state.range(0) + state.range(1), [&] {
        const std::size_t producers_count = state.range(0);
        const std::size_t consumers_count = state.range(1);
        const std::size_t max_size = state.range(2);

        auto queue = QueueType::Create(max_size);
        std::atomic<bool> run{true};

        std::vector<engine::TaskWithResult<void>> producer_tasks;
        producer_tasks.reserve(producers_count);
        for (std::size_t i = 0; i < producers_count; ++i) {
            producer_tasks.push_back(GetProducerTask(queue->GetProducer(), run));
        }

        std::vector<engine::TaskWithResult<void>> consumer_tasks;
        consumer_tasks.reserve(consumers_count - 1);
        for (std::size_t i = 0; i < consumers_count - 1; ++i) {
            consumer_tasks.push_back(GetConsumerTask(queue->GetConsumer()));
        }

        {
            std::size_t message = 0;
            auto consumer = queue->GetConsumer();
            for ([[maybe_unused]] auto _ : state) {
                (void)consumer.Pop(message);
                benchmark::DoNotOptimize(message);
            }
        }

        run = false;
        for (auto& task : producer_tasks) {
            task.RequestCancel();
            task.Get();
        }
        for (auto& task : consumer_tasks) {
            task.Get();
        }
    });
}

BENCHMARK_TEMPLATE(QueueProduce, concurrent::NonFifoMpmcQueue<std::size_t>)
    ->RangeMultiplier(2)
    ->Ranges({{1, 1}, {1, 4}, {128, 512}});

BENCHMARK_TEMPLATE(QueueProduce, concurrent::NonFifoMpmcQueue<std::size_t>)
    ->RangeMultiplier(2)
    ->Ranges({{1, 4}, {1, 4}, {kUnbounded, kUnbounded}});

BENCHMARK_TEMPLATE(QueueProduce, concurrent::impl::UnfairUnboundedNonFifoMpmcQueue<std::size_t>)
    ->RangeMultiplier(2)
    ->Ranges({{1, 4}, {1, 4}, {kUnbounded, kUnbounded}});

BENCHMARK_TEMPLATE(QueueConsume, concurrent::NonFifoMpmcQueue<std::size_t>)
    ->RangeMultiplier(2)
    ->Ranges({{1, 4}, {1, 1}, {128, 512}});

BENCHMARK_TEMPLATE(QueueConsume, concurrent::NonFifoMpmcQueue<std::size_t>)
    ->RangeMultiplier(2)
    ->Ranges({{1, 4}, {1, 1}, {kUnbounded, kUnbounded}});

BENCHMARK_TEMPLATE(QueueConsume, concurrent::impl::UnfairUnboundedNonFifoMpmcQueue<std::size_t>)
    ->RangeMultiplier(2)
    ->Ranges({{1, 4}, {1, 1}, {kUnbounded, kUnbounded}});

BENCHMARK_TEMPLATE(QueueProduce, concurrent::NonFifoMpscQueue<std::size_t>)
    ->RangeMultiplier(2)
    ->Ranges({{1, 4}, {1, 1}, {kUnbounded, kUnbounded}});

BENCHMARK_TEMPLATE(QueueProduce, concurrent::UnboundedNonFifoMpscQueue<std::size_t>)
    ->RangeMultiplier(2)
    ->Ranges({{1, 4}, {1, 1}, {kUnbounded, kUnbounded}});

BENCHMARK_TEMPLATE(QueueConsume, concurrent::NonFifoMpscQueue<std::size_t>)
    ->RangeMultiplier(2)
    ->Ranges({{1, 4}, {1, 1}, {kUnbounded, kUnbounded}});

BENCHMARK_TEMPLATE(QueueConsume, concurrent::UnboundedNonFifoMpscQueue<std::size_t>)
    ->RangeMultiplier(2)
    ->Ranges({{1, 4}, {1, 1}, {kUnbounded, kUnbounded}});

BENCHMARK_TEMPLATE(QueueProduce, concurrent::SpmcQueue<std::size_t>)
    ->RangeMultiplier(2)
    ->Ranges({{1, 1}, {1, 4}, {kUnbounded, kUnbounded}});

BENCHMARK_TEMPLATE(QueueProduce, concurrent::UnboundedSpmcQueue<std::size_t>)
    ->RangeMultiplier(2)
    ->Ranges({{1, 1}, {1, 4}, {kUnbounded, kUnbounded}});

BENCHMARK_TEMPLATE(QueueConsume, concurrent::SpmcQueue<std::size_t>)
    ->RangeMultiplier(2)
    ->Ranges({{1, 1}, {1, 1}, {kUnbounded, kUnbounded}});

BENCHMARK_TEMPLATE(QueueConsume, concurrent::UnboundedSpmcQueue<std::size_t>)
    ->RangeMultiplier(2)
    ->Ranges({{1, 1}, {1, 1}, {kUnbounded, kUnbounded}});

BENCHMARK_TEMPLATE(QueueProduce, concurrent::SpscQueue<std::size_t>)
    ->RangeMultiplier(2)
    ->Ranges({{1, 1}, {1, 1}, {kUnbounded, kUnbounded}});

BENCHMARK_TEMPLATE(QueueProduce, concurrent::UnboundedSpscQueue<std::size_t>)
    ->RangeMultiplier(2)
    ->Ranges({{1, 1}, {1, 1}, {kUnbounded, kUnbounded}});

BENCHMARK_TEMPLATE(QueueConsume, concurrent::SpscQueue<std::size_t>)
    ->RangeMultiplier(2)
    ->Ranges({{1, 1}, {1, 1}, {kUnbounded, kUnbounded}});

BENCHMARK_TEMPLATE(QueueConsume, concurrent::UnboundedSpscQueue<std::size_t>)
    ->RangeMultiplier(2)
    ->Ranges({{1, 1}, {1, 1}, {kUnbounded, kUnbounded}});

BENCHMARK_TEMPLATE(QueueProduce, concurrent::MpscQueue<std::size_t>)
    ->RangeMultiplier(2)
    ->Ranges({{1, 1}, {1, 1}, {128, 512}});

BENCHMARK_TEMPLATE(QueueProduce, concurrent::MpscQueue<std::size_t>)
    ->RangeMultiplier(2)
    ->Ranges({{1, 4}, {1, 1}, {kUnbounded, kUnbounded}});

BENCHMARK_TEMPLATE(QueueConsume, concurrent::MpscQueue<std::size_t>)
    ->RangeMultiplier(2)
    ->Ranges({{1, 4}, {1, 1}, {128, 512}});

BENCHMARK_TEMPLATE(QueueConsume, concurrent::MpscQueue<std::size_t>)
    ->RangeMultiplier(2)
    ->Ranges({{1, 4}, {1, 1}, {kUnbounded, kUnbounded}});

USERVER_NAMESPACE_END
