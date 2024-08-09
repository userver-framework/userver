#include <benchmark/benchmark.h>

#include <unistd.h>

#include <userver/engine/run_standalone.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/utils/fixed_array.hpp>
#include <utils/check_syscall.hpp>

#include "watcher.hpp"

USERVER_NAMESPACE_BEGIN

namespace {

class Pipe final {
 public:
  Pipe() { utils::CheckSyscall(::pipe(fd_), "creating pipe"); }
  ~Pipe() {
    if (fd_[0] != -1) ::close(fd_[0]);
    if (fd_[1] != -1) ::close(fd_[1]);
  }

  int GetIn() { return fd_[0]; }
  int GetOut() { return fd_[1]; }

 private:
  int fd_[2]{};
};

void NoInvokeCallback(struct ev_loop*, ev_io*, int) noexcept { UASSERT(false); }

namespace ev = engine::ev;

}  // namespace

void watcher_async_start(benchmark::State& state) {
  engine::RunStandalone([&]() {
    Pipe pipe;
    ev::Watcher<ev_io> watcher{engine::current_task::GetEventThread(), &pipe};
    watcher.Init(NoInvokeCallback, pipe.GetIn(), EV_READ);

    for ([[maybe_unused]] auto _ : state) {
      watcher.StartAsync();
      watcher.Stop();
    }
  });
}
BENCHMARK(watcher_async_start);

void watcher_async_start_multiple(benchmark::State& state) {
  engine::RunStandalone([&]() {
    static constexpr unsigned kPipes = 8;
    Pipe pipes[kPipes];
    utils::FixedArray<ev::Watcher<ev_io>> watchers(
        kPipes * 2, engine::current_task::GetEventThread(), &pipes[0]);
    for (unsigned i = 0; i < kPipes; ++i) {
      watchers[i * 2 + 0].Init(NoInvokeCallback, pipes[i].GetIn(), EV_READ);
      watchers[i * 2 + 1].Init(NoInvokeCallback, pipes[i].GetOut(), EV_READ);
    }

    for ([[maybe_unused]] auto _ : state) {
      for (auto& watcher : watchers) {
        watcher.StartAsync();
      }
      for (auto& watcher : watchers) {
        watcher.Stop();
      }
    }
  });
}
BENCHMARK(watcher_async_start_multiple);

USERVER_NAMESPACE_END
