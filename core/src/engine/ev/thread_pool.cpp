#include "thread_pool.hpp"

#include <fmt/format.h>

#include <userver/utils/assert.hpp>

#include "thread.hpp"
#include "thread_control.hpp"

USERVER_NAMESPACE_BEGIN

namespace engine::ev {

ThreadPool::ThreadPool(ThreadPoolConfig config)
    : ThreadPool(std::move(config), false) {}

ThreadPool::ThreadPool(ThreadPoolConfig config, UseDefaultEvLoop)
    : ThreadPool(std::move(config), !config.ev_default_loop_disabled) {}

ThreadPool::ThreadPool(ThreadPoolConfig config, bool use_ev_default_loop)
    : use_ev_default_loop_(use_ev_default_loop) {
  const auto register_timer_event_mode = Thread::RegisterEventMode::kImmediate;

  {
    default_threads_.threads =
        utils::GenerateFixedArray(config.threads, [&](std::size_t index) {
          const auto thread_name =
              fmt::format("{}_{}", config.thread_name, index);
          return (use_ev_default_loop && index == 0)
                     ? Thread(thread_name, Thread::kUseDefaultEvLoop,
                              register_timer_event_mode)
                     : Thread(thread_name, register_timer_event_mode);
        });

    default_threads_.thread_controls = utils::GenerateFixedArray(
        default_threads_.threads.size(), [&](std::size_t index) {
          return ThreadControl(default_threads_.threads[index]);
        });
  }

  {
    timer_threads_.threads = utils::GenerateFixedArray(
        config.dedicated_timer_threads, [](std::size_t index) {
          return Thread{fmt::format("ev-timer_{}", index),
                        Thread::RegisterEventMode::kDedicatedDeferred};
        });

    // Although we expect to always have a dedicated timer thread[s]
    // in the future, this feature is experimental for now and not always on.
    // So we operate over our own timer threads if they are present, or fallback
    // to default threads, behaving almost (separate atomic indexes, everything
    // besides stays exactly the same) identically to what we had before.
    auto& threads_to_wrap = timer_threads_.threads.empty()
                                ? default_threads_.threads
                                : timer_threads_.threads;
    timer_threads_.thread_controls = utils::GenerateFixedArray(
        threads_to_wrap.size(), [&threads_to_wrap](std::size_t index) {
          return TimerThreadControl{threads_to_wrap[index]};
        });
  }
}

ThreadPool::~ThreadPool() {
  // DATA RACE if not stopping the threads first.
  // Pointer to TimerThreadControl could be in execution queue of thread or
  // waiting for ev loop event. Stopping the threads makes sure that no pointer
  // to *Control is pending.
  timer_threads_.threads = {};
  default_threads_.threads = {};
}

std::size_t ThreadPool::GetSize() const {
  return default_threads_.threads.size();
}

ThreadControl& ThreadPool::NextThread() { return default_threads_.Next(); }

TimerThreadControl& ThreadPool::NextTimerThread() {
  return timer_threads_.Next();
}

ThreadControl& ThreadPool::GetEvDefaultLoopThread() {
  UINVARIANT(!default_threads_.Empty() && use_ev_default_loop_,
             "no ev_default_loop in current thread_pool");
  return default_threads_.thread_controls[0];
}

void ThreadPool::WriteStats(utils::statistics::Writer& writer) const {
  for (auto& thread : default_threads_.threads) {
    writer.ValueWithLabels(thread.GetCurrentLoadPercent(),
                           {{"ev_thread_name", thread.GetName()}});
  }
}

}  // namespace engine::ev

USERVER_NAMESPACE_END
