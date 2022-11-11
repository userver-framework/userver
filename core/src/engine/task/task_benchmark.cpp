#include <benchmark/benchmark.h>

#include <thread>

#include <engine/task/task_context.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/run_standalone.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/engine/task/task_with_result.hpp>

USERVER_NAMESPACE_BEGIN

void engine_task_create(benchmark::State& state) {
  engine::RunStandalone([&] {
    for (auto _ : state) engine::AsyncNoSpan([]() {}).Detach();
  });
}
BENCHMARK(engine_task_create);

void engine_task_create_destroy(benchmark::State& state) {
  engine::RunStandalone([&] {
    for (auto _ : state) {
      auto task = engine::AsyncNoSpan([]() {});
      task.Get();
    }
  });
}
BENCHMARK(engine_task_create_destroy);

void engine_task_yield(benchmark::State& state) {
  engine::RunStandalone([&] {
    std::vector<engine::TaskWithResult<void>> tasks;
    for (int i = 0; i < state.range(0); i++)
      tasks.push_back(engine::AsyncNoSpan([]() {
        while (!engine::current_task::ShouldCancel()) engine::Yield();
      }));

    for (auto _ : state) engine::Yield();
  });
}
BENCHMARK(engine_task_yield)->Arg(0)->RangeMultiplier(2)->Range(1, 10);

void engine_task_yield_multiple_threads(benchmark::State& state) {
  engine::RunStandalone(state.range(0), [&] {
    std::vector<engine::TaskWithResult<void>> tasks;
    for (int i = 0; i < state.range(0) - 1; i++)
      tasks.push_back(engine::AsyncNoSpan([]() {
        while (!engine::current_task::ShouldCancel()) engine::Yield();
      }));

    for (auto _ : state) engine::Yield();
  });
}
BENCHMARK(engine_task_yield_multiple_threads)->RangeMultiplier(2)->Range(1, 32);

void thread_yield(benchmark::State& state) {
  for (auto _ : state) std::this_thread::yield();
}
BENCHMARK(thread_yield)->RangeMultiplier(2)->ThreadRange(1, 32);

namespace {

template <size_t PayloadSize>
struct Payload final {
  void operator()() {}

  char data[PayloadSize];
};

template <size_t PayloadSize>
auto MakeContext() {
  auto holder =
      engine::impl::TaskContextHolder::Allocate(Payload<PayloadSize>{});

  new (holder.storage.get())
      engine::impl::TaskContext{engine::current_task::GetTaskProcessor(),
                                engine::Task::Importance::kNormal,
                                engine::Task::WaitMode::kSingleWaiter,
                                {},
                                holder.payload};

  return boost::intrusive_ptr<engine::impl::TaskContext>{
      static_cast<engine::impl::TaskContext*>(
          static_cast<void*>(holder.storage.release()))};
}

}  // namespace

template <size_t PayloadSize>
void task_context_allocate(benchmark::State& state) {
  engine::RunStandalone([&state] {
    for (auto _ : state) {
      benchmark::DoNotOptimize(MakeContext<PayloadSize>());
    }
  });
}
BENCHMARK_TEMPLATE(task_context_allocate, 32);
BENCHMARK_TEMPLATE(task_context_allocate, 64);
BENCHMARK_TEMPLATE(task_context_allocate, 128);
BENCHMARK_TEMPLATE(task_context_allocate, 256);
BENCHMARK_TEMPLATE(task_context_allocate, 512);

USERVER_NAMESPACE_END
