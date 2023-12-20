#include "thread_pool.hpp"

#include <fmt/format.h>

#include <userver/utils/assert.hpp>

#include "thread.hpp"
#include "thread_control.hpp"

USERVER_NAMESPACE_BEGIN

namespace engine::ev {

namespace {

Thread::RegisterEventMode GetRegisterEventMode(bool defer_timers) {
  return defer_timers ? Thread::RegisterEventMode::kDeferred
                      : Thread::RegisterEventMode::kImmediate;
}

}  // namespace

ThreadPool::ThreadPool(ThreadPoolConfig config)
    : ThreadPool(std::move(config), false) {}

ThreadPool::ThreadPool(ThreadPoolConfig config, UseDefaultEvLoop)
    : ThreadPool(std::move(config), !config.ev_default_loop_disabled) {}

ThreadPool::ThreadPool(ThreadPoolConfig config, bool use_ev_default_loop)
    : use_ev_default_loop_(use_ev_default_loop) {
  const auto register_timer_event_mode =
      GetRegisterEventMode(config.defer_events);

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
                        Thread::RegisterEventMode::kDeferred};
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

std::vector<ThreadControl*> ThreadPool::NextThreads(std::size_t count) {
  std::vector<ThreadControl*> res;
  if (!count) return res;
  UASSERT(!default_threads_.Empty());
  res.reserve(count);

  // just ignore counter_ overflow
  const auto start = default_threads_.next_thread_idx.fetch_add(count);
  for (std::size_t i = 0; i < count; i++) {
    res.push_back(
        &default_threads_
             .thread_controls[(start + i) %
                              default_threads_.thread_controls.size()]);
  }
  return res;
}

TimerThreadControl& ThreadPool::NextTimerThread() {
  return timer_threads_.Next();
}

ThreadControl& ThreadPool::GetEvDefaultLoopThread() {
  UINVARIANT(!default_threads_.Empty() && use_ev_default_loop_,
             "no ev_default_loop in current thread_pool");
  return default_threads_.thread_controls[0];
}

}  // namespace engine::ev

USERVER_NAMESPACE_END
