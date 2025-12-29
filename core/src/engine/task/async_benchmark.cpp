#include <benchmark/benchmark.h>

#include <array>
#include <thread>

#include <userver/engine/async.hpp>
#include <userver/engine/impl/task_local_storage.hpp>
#include <userver/engine/run_standalone.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/fixed_array.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

using WrappedSpanCall = utils::impl::WrappedCallImplType<decltype(utils::impl::SpanLazyPrvalue("")), void (*)()>;

}

// Note: We intentionally do not run this benchmark from RunStandalone to avoid
// any side-effects (RunStandalone spawns additional std::threads and uses some
// synchronization primitives).
void AsyncComparisonsStdThread(benchmark::State& state) {
    std::uint64_t constructed_joined_count = 0;
    for ([[maybe_unused]] auto _ : state) {
        std::thread([] {}).join();
        ++constructed_joined_count;
    }
    benchmark::DoNotOptimize(constructed_joined_count);
}
BENCHMARK(AsyncComparisonsStdThread);

template <engine::TaskQueueType Scheduler>
void AsyncComparisonsCoro(benchmark::State& state) {
    engine::TaskProcessorPoolsConfig config{};
    config.queue_type = Scheduler;
    engine::RunStandalone(state.range(0), config, [&] {
        std::uint64_t constructed_joined_count = 0;
        for ([[maybe_unused]] auto _ : state) {
            engine::AsyncNoSpan([] {}).Wait();
            ++constructed_joined_count;
        }
        benchmark::DoNotOptimize(constructed_joined_count);
    });
}

void AsyncComparisonsCoroDefault(benchmark::State& state) {
    AsyncComparisonsCoro<engine::TaskQueueType::kGlobalTaskQueue>(state);
}
BENCHMARK(AsyncComparisonsCoroDefault)->RangeMultiplier(2)->Range(1, 32);

void AsyncComparisonsCoroPullPin(benchmark::State& state) {
    AsyncComparisonsCoro<engine::TaskQueueType::kPullPinTaskQueue>(state);
}
BENCHMARK(AsyncComparisonsCoroPullPin)->RangeMultiplier(2)->Range(1, 32);

template <engine::TaskQueueType Scheduler>
void WrapCallSingle(benchmark::State& state) {
    engine::TaskProcessorPoolsConfig config{};
    config.queue_type = Scheduler;
    engine::RunStandalone(1, config, [&] {
        for ([[maybe_unused]] auto _ : state) {
            WrappedSpanCall(utils::impl::SpanLazyPrvalue(""), []() {});
        }
    });
}

void WrapCallSingleDefault(benchmark::State& state) { WrapCallSingle<engine::TaskQueueType::kGlobalTaskQueue>(state); }
BENCHMARK(WrapCallSingleDefault);

void WrapCallSinglePullPin(benchmark::State& state) { WrapCallSingle<engine::TaskQueueType::kPullPinTaskQueue>(state); }
BENCHMARK(WrapCallSinglePullPin);

template <engine::TaskQueueType Scheduler>
void WrapCallMultiple(benchmark::State& state) {
    engine::TaskProcessorPoolsConfig config{};
    config.queue_type = Scheduler;
    engine::RunStandalone(1, config, [&] {
        constexpr std::size_t kInMemoryInstancesCount = 100;
        utils::FixedArray<std::optional<WrappedSpanCall>> calls{kInMemoryInstancesCount};

        for ([[maybe_unused]] auto _ : state) {
            for (auto& call : calls) {
                call.emplace(utils::impl::SpanLazyPrvalue(""), []() {});
            }
            for (auto& call : calls) {
                call.reset();
            }
        }
    });
}

void WrapCallMultipleDefault(benchmark::State& state) {
    WrapCallMultiple<engine::TaskQueueType::kGlobalTaskQueue>(state);
}
BENCHMARK(WrapCallMultipleDefault);

void WrapCallMultiplePullPin(benchmark::State& state) {
    WrapCallMultiple<engine::TaskQueueType::kPullPinTaskQueue>(state);
}
BENCHMARK(WrapCallMultiplePullPin);

template <engine::TaskQueueType Scheduler>
void WrapCallAndPerform(benchmark::State& state) {
    engine::TaskProcessorPoolsConfig config{};
    config.queue_type = Scheduler;
    engine::RunStandalone(1, config, [&] {
        for ([[maybe_unused]] auto _ : state) {
            WrappedSpanCall wrapped_call{utils::impl::SpanLazyPrvalue(""), []() {}};
            {
                // Perform requires that task-local storage is empty, then fills it
                engine::impl::task_local::Storage discarded_storage;
                discarded_storage.InitializeFrom(std::move(engine::impl::task_local::GetCurrentStorage()));
            }
            wrapped_call.Perform();
        }
    });
}

void WrapCallAndPerformDefault(benchmark::State& state) {
    WrapCallAndPerform<engine::TaskQueueType::kGlobalTaskQueue>(state);
}
BENCHMARK(WrapCallAndPerformDefault);

void WrapCallAndPerformPullPin(benchmark::State& state) {
    WrapCallAndPerform<engine::TaskQueueType::kPullPinTaskQueue>(state);
}
BENCHMARK(WrapCallAndPerformPullPin);

template <engine::TaskQueueType Scheduler>
void AsyncComparisonsCoroSpanned(benchmark::State& state) {
    engine::TaskProcessorPoolsConfig config{};
    config.queue_type = Scheduler;
    engine::RunStandalone(state.range(0), config, [&] {
        std::uint64_t constructed_joined_count = 0;
        for ([[maybe_unused]] auto _ : state) {
            utils::Async("", [] {}).Wait();
            ++constructed_joined_count;
        }
        benchmark::DoNotOptimize(constructed_joined_count);
    });
}

void AsyncComparisonsCoroSpannedDefault(benchmark::State& state) {
    AsyncComparisonsCoroSpanned<engine::TaskQueueType::kGlobalTaskQueue>(state);
}
BENCHMARK(AsyncComparisonsCoroSpannedDefault)->RangeMultiplier(2)->Range(1, 32);

void AsyncComparisonsCoroSpannedPullPin(benchmark::State& state) {
    AsyncComparisonsCoroSpanned<engine::TaskQueueType::kPullPinTaskQueue>(state);
}
BENCHMARK(AsyncComparisonsCoroSpannedPullPin)->RangeMultiplier(2)->Range(1, 32);

USERVER_NAMESPACE_END
