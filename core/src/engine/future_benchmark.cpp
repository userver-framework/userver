#include <userver/engine/future.hpp>

#include <condition_variable>
#include <future>
#include <mutex>

#include <benchmark/benchmark.h>

#include <userver/engine/async.hpp>
#include <userver/engine/run_standalone.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class BlockingEvent final {
 public:
  BlockingEvent() = default;

  void Wait() {
    std::unique_lock lock(mutex_);
    cv_.wait(lock, [&] { return done_; });
  }

  void Send() {
    {
      std::lock_guard lock(mutex_);
      done_ = true;
    }
    cv_.notify_one();
  }

 private:
  std::mutex mutex_;
  std::condition_variable cv_;
  bool done_{false};
};

constexpr std::size_t kBatchSize = 1000;

template <typename Benchmark>
void RunPrepared(benchmark::State& state) {
  while (state.KeepRunningBatch(kBatchSize)) {
    state.PauseTiming();
    for (std::size_t i = 0; i < kBatchSize; ++i) {
      Benchmark benchmark;
      state.ResumeTiming();
      benchmark.Run();
      state.PauseTiming();
    }
    state.ResumeTiming();
  }
}

struct FutureStdSetGet {
  FutureStdSetGet() : future(promise.get_future()) {
    producer = std::thread([&] {
      producer_ready.Send();
      consumer_ready.Wait();
      promise.set_value(42);
    });
    producer_ready.Wait();
  }

  ~FutureStdSetGet() { producer.join(); }

  void Run() {
    consumer_ready.Send();
    benchmark::DoNotOptimize(future.get());
  }

  BlockingEvent producer_ready;
  BlockingEvent consumer_ready;
  std::promise<int> promise;
  std::future<int> future;
  std::thread producer;
};

struct FutureCoroSetGet {
  FutureCoroSetGet() : future(promise.get_future()) {
    producer = engine::AsyncNoSpan([&] {
      producer_ready.Send();
      const bool status = consumer_ready.WaitForEvent();
      UASSERT(status);
      promise.set_value(42);
    });
    const bool status = producer_ready.WaitForEvent();
    UASSERT(status);
  }

  void Run() {
    consumer_ready.Send();
    benchmark::DoNotOptimize(future.get());
  }

  engine::SingleConsumerEvent producer_ready;
  engine::SingleConsumerEvent consumer_ready;
  engine::Promise<int> promise;
  engine::Future<int> future;
  engine::TaskWithResult<void> producer;
};

}  // namespace

void future_std_single_threaded(benchmark::State& state) {
  for (auto _ : state) {
    std::promise<int> promise;
    auto future = promise.get_future();
    promise.set_value(42);
    benchmark::DoNotOptimize(future.get());
  }
}
BENCHMARK(future_std_single_threaded);

void future_coro_single_threaded(benchmark::State& state) {
  engine::RunStandalone([&] {
    for (auto _ : state) {
      engine::Promise<int> promise;
      auto future = promise.get_future();
      promise.set_value(42);
      benchmark::DoNotOptimize(future.get());
    }
  });
}
BENCHMARK(future_coro_single_threaded);

void future_std_set_and_get(benchmark::State& state) {
  engine::RunStandalone(2, [&] { RunPrepared<FutureStdSetGet>(state); });
}
BENCHMARK(future_std_set_and_get);

void future_coro_set_and_get(benchmark::State& state) {
  engine::RunStandalone(2, [&] { RunPrepared<FutureCoroSetGet>(state); });
}
BENCHMARK(future_coro_set_and_get);

USERVER_NAMESPACE_END
