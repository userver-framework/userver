#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include <ev.h>
#include <boost/lockfree/queue.hpp>
#include <boost/smart_ptr/intrusive_ptr.hpp>

#include <userver/engine/deadline.hpp>

#include <engine/ev/async_payload_base.hpp>
#include <utils/statistics/thread_statistics.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::ev {

class Thread final {
 public:
  struct UseDefaultEvLoop {};
  static constexpr UseDefaultEvLoop kUseDefaultEvLoop{};

  enum class RegisterEventMode {
    // With this mode RegisterEventInEvLoop will notify ev-loop right away,
    // behaving exactly as RunInEvLoopAsync.
    kImmediate,
    // With this mode RegisterEventInEvLoop will defer events execution to
    // a periodic timer, running with ~1ms resolution. It helps to avoid
    // the ev_async_send call, which incurs very noticeable overhead, however
    // event execution becomes delayed for a aforementioned ~1ms.
    kDeferred
  };

  Thread(const std::string& thread_name, RegisterEventMode);
  Thread(const std::string& thread_name, UseDefaultEvLoop, RegisterEventMode);
  ~Thread();

  struct ev_loop* GetEvLoop() const {
    return loop_;
  }

  // Callbacks passed to RunInEvLoopAsync() are serialized.
  // All successfully registered callbacks are guaranteed to execute.
  void RunInEvLoopAsync(OnAsyncPayload* func, AsyncPayloadPtr&& data);

  // Callbacks passed to RunInEvLoopDeferred() are serialized.
  // Same as RunInEvLoopAsync but doesn't force the wakeup of ev-loop, adding
  // delay up to ~1ms.
  void RunInEvLoopDeferred(OnAsyncPayload* func, AsyncPayloadPtr&& data,
                           Deadline deadline);

  bool IsInEvThread() const;

  std::uint8_t GetCurrentLoadPct() const;
  const std::string& GetName() const;

 private:
  Thread(const std::string& thread_name, bool use_ev_default_loop,
         RegisterEventMode register_event_mode);

  void RegisterInEvLoop(OnAsyncPayload* func, AsyncPayloadPtr&& data);

  void Start(const std::string& name);

  void StopEventLoop();
  void RunEvLoop();

  static void UpdateLoopWatcher(struct ev_loop*, ev_async* w, int) noexcept;
  static void UpdateTimersWatcher(struct ev_loop*, ev_timer* w, int) noexcept;
  void UpdateLoopWatcherImpl();
  static void BreakLoopWatcher(struct ev_loop*, ev_async* w, int) noexcept;
  void BreakLoopWatcherImpl();
  static void ChildWatcher(struct ev_loop*, ev_child* w, int) noexcept;
  static void ChildWatcherImpl(ev_child* w);

  static void Acquire(struct ev_loop* loop) noexcept;
  static void Release(struct ev_loop* loop) noexcept;
  void AcquireImpl() noexcept;
  void ReleaseImpl() noexcept;

  bool use_ev_default_loop_;
  RegisterEventMode register_event_mode_;

  struct QueueData {
    OnAsyncPayload* func;
    AsyncPayloadBase* data;
  };

  boost::lockfree::queue<QueueData> func_queue_;

  struct ev_loop* loop_;
  std::thread thread_;
  std::mutex loop_mutex_;
  std::unique_lock<std::mutex> lock_;

  ev_timer timers_driver_{};
  ev_async watch_update_{};
  ev_async watch_break_{};
  ev_child watch_child_{};

  std::string name_;
  utils::statistics::ThreadCpuStatsStorage cpu_stats_storage_;

  bool is_running_;
};

}  // namespace engine::ev

USERVER_NAMESPACE_END
