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
  threads_.reserve(config.threads);
  const auto register_timer_event_mode =
      GetRegisterEventMode(config.defer_events);
  for (size_t i = 0; i < config.threads; i++) {
    const auto thread_name = fmt::format("{}_{}", config.thread_name, i);
    threads_.push_back(
        use_ev_default_loop_ && !i
            ? std::make_unique<Thread>(thread_name, Thread::kUseDefaultEvLoop,
                                       register_timer_event_mode)
            : std::make_unique<Thread>(thread_name, register_timer_event_mode));
  }

  thread_controls_.reserve(threads_.size());
  for (const auto& thread : threads_) {
    thread_controls_.emplace_back(*thread);
  }
}

ThreadPool::~ThreadPool() = default;

std::size_t ThreadPool::GetSize() const { return threads_.size(); }

ThreadControl& ThreadPool::NextThread() {
  UASSERT(!thread_controls_.empty());
  // just ignore counter_ overflow
  return thread_controls_[next_thread_idx_++ % thread_controls_.size()];
}

std::vector<ThreadControl*> ThreadPool::NextThreads(std::size_t count) {
  std::vector<ThreadControl*> res;
  if (!count) return res;
  UASSERT(!thread_controls_.empty());
  res.reserve(count);
  // just ignore counter_ overflow
  const auto start = next_thread_idx_.fetch_add(count);
  for (std::size_t i = 0; i < count; i++) {
    res.push_back(&thread_controls_[(start + i) % thread_controls_.size()]);
  }
  return res;
}

ThreadControl& ThreadPool::GetEvDefaultLoopThread() {
  UINVARIANT(!thread_controls_.empty() && use_ev_default_loop_,
             "no ev_default_loop in current thread_pool");
  return thread_controls_[0];
}

}  // namespace engine::ev

USERVER_NAMESPACE_END
