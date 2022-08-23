#include <benchmark/benchmark.h>

#include <atomic>

#include <userver/engine/async.hpp>
#include <userver/engine/run_standalone.hpp>
#include <userver/utils/fixed_array.hpp>
#include <userver/utils/impl/wrapped_call.hpp>
#include <userver/utils/make_intrusive_ptr.hpp>

#include <engine/impl/wait_list.hpp>
#include <engine/task/task_context.hpp>

USERVER_NAMESPACE_BEGIN

using engine::impl::TaskContext;
using engine::impl::WaitList;
using engine::impl::WaitScope;

namespace {

constexpr std::size_t kTasksCount = 1024 * 64;
constexpr std::size_t kIterationsCount = 1024 * 16;

engine::impl::TaskPayload MakeEmptyPayload() {
  return utils::impl::WrapCall([] {});
}

auto MakeContext() {
  return utils::make_intrusive_ptr<TaskContext>(
      engine::current_task::GetTaskProcessor(),
      engine::Task::Importance::kNormal, engine::Task::WaitMode::kSingleWaiter,
      engine::Deadline{}, MakeEmptyPayload());
}

auto MakeWaitScopes(WaitList& wl, TaskContext& context) {
  return utils::FixedArray<WaitScope>(kTasksCount, wl, context);
}

}  // namespace

void wait_list_insertion(benchmark::State& state) {
  engine::RunStandalone([&] {
    std::size_t i = 0;
    WaitList wl;
    auto& context = engine::current_task::GetCurrentTaskContext();
    auto wait_scopes = MakeWaitScopes(wl, context);
    {
      for (auto _ : state) {
        wait_scopes[i].Append();

        if (++i == kTasksCount) {
          state.PauseTiming();
          while (i--) {
            wait_scopes[i].Remove();
          }
          state.ResumeTiming();
        }
      }
    }

    while (i--) {
      wait_scopes[i].Remove();
    }
  });
}
BENCHMARK(wait_list_insertion)->Iterations(kIterationsCount);

void wait_list_removal(benchmark::State& state) {
  engine::RunStandalone([&] {
    WaitList wl;
    auto& context = engine::current_task::GetCurrentTaskContext();
    auto wait_scopes = MakeWaitScopes(wl, context);

    for (auto& scope : wait_scopes) {
      scope.Append();
    }

    std::size_t i = 0;
    for (auto _ : state) {
      wait_scopes[i].Remove();

      if (++i == kTasksCount) {
        state.PauseTiming();
        i = 0;
        {
          for (auto& scope : wait_scopes) {
            scope.Append();
          }
        }
        state.ResumeTiming();
      }
    }

    while (i != kTasksCount) {
      wait_scopes[i].Remove();
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
        auto ctx = MakeContext();
        WaitScope wait_scope(wl, *ctx);
        while (run) {
          wait_scope.Append();
          wait_scope.Remove();
        }
      }));

    auto ctx = MakeContext();
    WaitScope wait_scope(wl, *ctx);

    for (auto _ : state) {
      wait_scope.Append();
      wait_scope.Remove();
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
        auto& context = engine::current_task::GetCurrentTaskContext();
        auto wait_scopes = MakeWaitScopes(wl, context);

        while (run) {
          for (auto& scope : wait_scopes) {
            scope.Append();
          }
          for (auto& scope : wait_scopes) {
            scope.Remove();
          }
        }
      }));

    auto& context = engine::current_task::GetCurrentTaskContext();
    auto wait_scopes = MakeWaitScopes(wl, context);

    for (auto _ : state) {
      for (auto& scope : wait_scopes) {
        scope.Append();
      }
      for (auto& scope : wait_scopes) {
        scope.Remove();
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
    ->Range(1, 4)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime();

USERVER_NAMESPACE_END
