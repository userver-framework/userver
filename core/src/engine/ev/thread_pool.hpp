#pragma once

#include <atomic>
#include <vector>

#include <engine/ev/thread_control.hpp>
#include <engine/ev/thread_pool_config.hpp>
#include <userver/utils/fixed_array.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::ev {

class Thread;

class ThreadPool final {
 public:
  struct UseDefaultEvLoop {};
  static constexpr UseDefaultEvLoop kUseDefaultEvLoop{};

  explicit ThreadPool(ThreadPoolConfig config);
  ThreadPool(ThreadPoolConfig config, UseDefaultEvLoop);

  ~ThreadPool();

  std::size_t GetSize() const;

  ThreadControl& NextThread();
  std::vector<ThreadControl*> NextThreads(std::size_t count);

  TimerThreadControl& NextTimerThread();

  ThreadControl& GetEvDefaultLoopThread();

 private:
  ThreadPool(ThreadPoolConfig config, bool use_ev_default_loop);

  bool use_ev_default_loop_;

  template <typename Control>
  struct BunchOfThreads final {
    utils::FixedArray<Thread> threads;
    utils::FixedArray<Control> thread_controls;
    std::atomic<std::size_t> next_thread_idx{0};

    Control& Next() {
      UASSERT(!thread_controls.empty());
      // just ignore counter_ overflow
      return thread_controls[next_thread_idx++ % thread_controls.size()];
    }
    bool Empty() const noexcept { return thread_controls.empty(); }
  };

  BunchOfThreads<ThreadControl> default_threads_;
  BunchOfThreads<TimerThreadControl> timer_threads_;
};

}  // namespace engine::ev

USERVER_NAMESPACE_END
