#include <benchmark/benchmark.h>

#include <atomic>
#include <thread>

#include <userver/engine/async.hpp>
#include <userver/engine/impl/task_context_holder.hpp>
#include <userver/engine/run_standalone.hpp>

#include <engine/impl/wait_list.hpp>
#include <engine/task/task_context.hpp>

USERVER_NAMESPACE_BEGIN

using engine::impl::TaskContext;
using engine::impl::WaitList;

namespace {

constexpr std::size_t kTasksCount = 1024 * 64;
constexpr std::size_t kIterationsCount = 1024 * 16;

boost::intrusive_ptr<TaskContext> MakeContext() {
  return engine::impl::MakeTask({engine::current_task::GetTaskProcessor(),
                                 engine::Task::Importance::kNormal,
                                 engine::Task::WaitMode::kSingleWaiter,
                                 {}},
                                [] {})
      .Extract();
}

auto MakeContexts() {
  std::vector<boost::intrusive_ptr<TaskContext>> contexts;
  contexts.reserve(kTasksCount);
  for (std::size_t i = 0; i < kTasksCount; ++i) {
    contexts.push_back(MakeContext());
  }

  return contexts;
}

}  // namespace

void wait_list_insertion(benchmark::State& state) {
  engine::RunStandalone([&] {
    std::size_t i = 0;
    WaitList wl;

    auto contexts = MakeContexts();
    {
      WaitList::Lock guard{wl};
      for (auto _ : state) {
        wl.Append(guard, contexts[i]);

        if (++i == kTasksCount) {
          state.PauseTiming();
          while (i--) {
            wl.Remove(guard, *contexts[i]);
          }
          state.ResumeTiming();
        }
      }
    }

    WaitList::Lock guard{wl};
    while (i--) {
      wl.Remove(guard, *contexts[i]);
    }
  });
}
BENCHMARK(wait_list_insertion)->Iterations(kIterationsCount);

void wait_list_removal(benchmark::State& state) {
  engine::RunStandalone([&] {
    WaitList wl;

    auto contexts = MakeContexts();

    WaitList::Lock guard{wl};
    for (auto c : contexts) {
      wl.Append(guard, c);
    }

    std::size_t i = 0;
    for (auto _ : state) {
      wl.Remove(guard, *contexts[i]);

      if (++i == kTasksCount) {
        state.PauseTiming();
        i = 0;
        {
          for (auto c : contexts) {
            wl.Append(guard, c);
          }
        }
        state.ResumeTiming();
      }
    }

    while (i != kTasksCount) {
      wl.Remove(guard, *contexts[i]);
      ++i;
    }
  });
}
BENCHMARK(wait_list_removal)->Iterations(kIterationsCount);

void wait_list_add_remove_contention(benchmark::State& state) {
  engine::RunStandalone(state.range(0), [&] {
    std::atomic<bool> run{true};
    WaitList wl;

    std::vector<engine::TaskWithResult<void>> tasks;
    for (int i = 0; i < state.range(0) - 1; i++)
      tasks.push_back(engine::AsyncNoSpan([&]() {
        boost::intrusive_ptr<TaskContext> ctx = MakeContext();
        while (run) {
          {
            WaitList::Lock guard{wl};
            wl.Append(guard, ctx);
          }
          WaitList::Lock guard{wl};
          wl.Remove(guard, *ctx);
        }
      }));

    boost::intrusive_ptr<TaskContext> ctx = MakeContext();
    for (auto _ : state) {
      {
        WaitList::Lock guard{wl};
        wl.Append(guard, ctx);
      }
      WaitList::Lock guard{wl};
      wl.Remove(guard, *ctx);
    }

    run = false;
  });
}
BENCHMARK(wait_list_add_remove_contention)
    ->RangeMultiplier(2)
    ->Range(1, 2)
    ->UseRealTime();

void wait_list_add_remove_contention_unbalanced(benchmark::State& state) {
  engine::RunStandalone(state.range(0), [&] {
    std::atomic<bool> run{true};
    WaitList wl;

    std::vector<engine::TaskWithResult<void>> tasks;
    for (int i = 0; i < state.range(0) - 1; i++)
      tasks.push_back(engine::AsyncNoSpan([&]() {
        auto contexts = MakeContexts();
        while (run) {
          for (auto& ctx : contexts) {
            WaitList::Lock guard{wl};
            wl.Append(guard, ctx);
          }
          for (auto& ctx : contexts) {
            WaitList::Lock guard{wl};
            wl.Remove(guard, *ctx);
          }
        }
      }));

    auto contexts = MakeContexts();
    for (auto _ : state) {
      for (auto& ctx : contexts) {
        WaitList::Lock guard{wl};
        wl.Append(guard, ctx);
      }
      for (auto& ctx : contexts) {
        WaitList::Lock guard{wl};
        wl.Remove(guard, *ctx);
      }
    }

    run = false;
  });
}

// This benchmark has been restricted to using only 1 thread, because a single
// iteration of it (the multithreaded version) could easily take about 10
// minutes on a modern CPU.
//
// That happened because each thread of the benchmark locks and unlocks the same
// std::mutex in a rapid succession. Most of the times, the thread, which
// previously owned the mutex, just re-locks it again without giving the other
// threads an opportunity to work on the WaitList. On top of it, for a benchmark
// iteration to complete, the rare ownership switch is required to have occurred
// A LOT of times (once per TaskContext).
BENCHMARK(wait_list_add_remove_contention_unbalanced)
    ->RangeMultiplier(2)
    ->Range(1, 1)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime();

USERVER_NAMESPACE_END
