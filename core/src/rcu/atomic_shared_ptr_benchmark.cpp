#include <benchmark/benchmark.h>

#include <atomic>
#include <memory>
#include <unordered_map>

#include <userver/engine/async.hpp>
#include <userver/engine/run_in_coro.hpp>
#include <userver/engine/sleep.hpp>

USERVER_NAMESPACE_BEGIN

template <typename T>
class AtomicSharedPtr {
 public:
  explicit AtomicSharedPtr(std::shared_ptr<const T> new_value)
      : storage_(std::move(new_value)) {}

  AtomicSharedPtr(AtomicSharedPtr&&) = delete;
  AtomicSharedPtr& operator=(AtomicSharedPtr&&) = delete;

  std::shared_ptr<const T> Load() const {
    return std::atomic_load_explicit(&storage_, std::memory_order_acquire);
  }

  void Assign(std::shared_ptr<const T>&& new_value) {
    std::atomic_store_explicit(&storage_, std::move(new_value),
                               std::memory_order_release);
  }

 private:
  std::shared_ptr<const T> storage_;
};

void atomic_shared_ptr_read(benchmark::State& state) {
  RunInCoro(
      [&]() {
        AtomicSharedPtr<int> ptr(std::make_unique<int>(1));

        for (auto _ : state) {
          auto snapshot_ptr = ptr.Load();
          benchmark::DoNotOptimize(*snapshot_ptr);
        }
      },
      1);
}
BENCHMARK(atomic_shared_ptr_read);

using std::literals::chrono_literals::operator""ms;

void atomic_shared_ptr_contention(benchmark::State& state) {
  RunInCoro(
      [&]() {
        std::atomic<bool> run{true};
        AtomicSharedPtr<std::unordered_map<int, int>> ptr{
            std::make_shared<std::unordered_map<int, int>>()};

        std::vector<engine::TaskWithResult<void>> tasks;
        for (int i = 0; i < state.range(0) - 2; i++)
          tasks.push_back(engine::impl::Async([&]() {
            while (run) {
              auto snapshot_ptr = ptr.Load();
              benchmark::DoNotOptimize(*snapshot_ptr);
            }
          }));

        if (state.range(1))
          tasks.push_back(engine::impl::Async([&]() {
            size_t i = 0;
            while (run) {
              std::unordered_map<int, int> writer = *ptr.Load();
              writer[1] = i++;
              ptr.Assign(std::make_shared<const std::unordered_map<int, int>>(
                  std::move(writer)));
              engine::SleepFor(10ms);
            }
          }));

        for (auto _ : state) {
          auto snapshot_ptr = ptr.Load();
          benchmark::DoNotOptimize(*snapshot_ptr);
        }

        run = false;
      },
      state.range(0));
}
BENCHMARK(atomic_shared_ptr_contention)
    ->RangeMultiplier(2)
    ->Ranges({{2, 32}, {false, true}});

USERVER_NAMESPACE_END
