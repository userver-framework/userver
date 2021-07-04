#include <benchmark/benchmark.h>

#include <atomic>
#include <thread>

#include <userver/engine/async.hpp>
#include <userver/engine/run_in_coro.hpp>

#include "task/task_context.hpp"
#include "wait_list.hpp"

#include <utils/gbench_auxilary.hpp>

namespace {
constexpr unsigned kTasksCount = 1024 * 64;
constexpr unsigned kIterationsCount = 1024 * 16;

using namespace engine::impl;

auto MakeContexts() {
  std::vector<boost::intrusive_ptr<TaskContext>> contexts;
  contexts.reserve(kTasksCount);
  for (unsigned i = 0; i < kTasksCount; ++i) {
    contexts.push_back(new TaskContext(engine::current_task::GetTaskProcessor(),
                                       engine::Task::Importance::kNormal, {},
                                       []() {}));
  }

  return contexts;
}

}  // namespace

void wait_list_insertion(benchmark::State& state) {
  RunInCoro(
      [&]() {
        unsigned i = 0;
        WaitList wl;

        auto contexts = MakeContexts();
        {
          WaitList::Lock guard{wl};
          for (auto _ : state) {
            wl.Append(guard, contexts[i]);

            if (++i == kTasksCount) {
              state.PauseTiming();
              while (i--) {
                wl.Remove(guard, contexts[i]);
              }
              state.ResumeTiming();
            }
          }
        }

        WaitList::Lock guard{wl};
        while (i--) {
          wl.Remove(guard, contexts[i]);
        }
      },
      1);
}
BENCHMARK(wait_list_insertion)->Iterations(kIterationsCount);

void wait_list_removal(benchmark::State& state) {
  using namespace engine::impl;
  RunInCoro(
      [&]() {
        WaitList wl;

        auto contexts = MakeContexts();

        WaitList::Lock guard{wl};
        for (auto c : contexts) {
          wl.Append(guard, c);
        }

        unsigned i = 0;
        for (auto _ : state) {
          wl.Remove(guard, contexts[i]);

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
          wl.Remove(guard, contexts[i]);
          ++i;
        }
      },
      1);
}
BENCHMARK(wait_list_removal)->Iterations(kIterationsCount);

void wait_list_add_remove_contention(benchmark::State& state) {
  using namespace engine::impl;
  RunInCoro(
      [&]() {
        std::atomic<bool> run{true};
        WaitList wl;

        std::vector<engine::TaskWithResult<void>> tasks;
        for (int i = 0; i < state.range(0) - 1; i++)
          tasks.push_back(engine::impl::Async([&]() {
            boost::intrusive_ptr<TaskContext> ctx =
                new TaskContext(engine::current_task::GetTaskProcessor(),
                                engine::Task::Importance::kNormal, {}, []() {});
            while (run) {
              {
                WaitList::Lock guard{wl};
                wl.Append(guard, ctx);
              }
              WaitList::Lock guard{wl};
              wl.Remove(guard, ctx);
            }
          }));

        boost::intrusive_ptr<TaskContext> ctx =
            new TaskContext(engine::current_task::GetTaskProcessor(),
                            engine::Task::Importance::kNormal, {}, []() {});
        for (auto _ : state) {
          {
            WaitList::Lock guard{wl};
            wl.Append(guard, ctx);
          }
          WaitList::Lock guard{wl};
          wl.Remove(guard, ctx);
        }

        run = false;
      },
      state.range(0));
}
BENCHMARK(wait_list_add_remove_contention)
    ->RangeMultiplier(2)
    ->Range(1, 2)
    ->UseRealTime();

void wait_list_add_remove_contention_unbalanced(benchmark::State& state) {
  using namespace engine::impl;
  RunInCoro(
      [&]() {
        std::atomic<bool> run{true};
        WaitList wl;

        std::vector<engine::TaskWithResult<void>> tasks;
        for (int i = 0; i < state.range(0) - 1; i++)
          tasks.push_back(engine::impl::Async([&]() {
            auto contexts = MakeContexts();
            while (run) {
              for (auto& ctx : contexts) {
                WaitList::Lock guard{wl};
                wl.Append(guard, ctx);
              }
              for (auto& ctx : contexts) {
                WaitList::Lock guard{wl};
                wl.Remove(guard, ctx);
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
            wl.Remove(guard, ctx);
          }
        }

        run = false;
      },
      state.range(0));
}
BENCHMARK(wait_list_add_remove_contention_unbalanced)
    ->RangeMultiplier(2)
    ->Range(1, 4)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime();
