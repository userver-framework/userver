#pragma once

#include <atomic>
#include <vector>

#include <engine/ev/thread_control.hpp>
#include <engine/ev/thread_pool_config.hpp>
#include <userver/utils/fixed_array.hpp>
#include <userver/utils/statistics/writer.hpp>

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

  TimerThreadControl& NextTimerThread();

  ThreadControl& GetEvDefaultLoopThread();

  friend void DumpMetric(utils::statistics::Writer& writer,
                         const ThreadPool& self) {
    self.WriteStats(writer);
  }

 private:
  ThreadPool(ThreadPoolConfig config, bool use_ev_default_loop);

  void WriteStats(utils::statistics::Writer& writer) const;

  template <typename Control>
  struct BunchOfControls final {
    utils::FixedArray<Control> controls;
    std::atomic<std::size_t> next_idx{0};

    Control& Next() noexcept {
      UASSERT(!controls.empty());
      // just ignore next_idx overflow
      return controls[next_idx++ % controls.size()];
    }

    bool Empty() const noexcept { return controls.empty(); }
  };

  BunchOfControls<ThreadControl> default_controls_;
  BunchOfControls<TimerThreadControl> timer_controls_;

  utils::FixedArray<Thread> threads_;

  const bool use_ev_default_loop_;
};

}  // namespace engine::ev

USERVER_NAMESPACE_END
