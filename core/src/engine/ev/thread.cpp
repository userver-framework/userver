#include "thread.hpp"

#include <chrono>
#include <stdexcept>

#include <sys/param.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <userver/compiler/demangle.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/datetime/steady_coarse_clock.hpp>
#include <userver/utils/thread_name.hpp>

#include <utils/check_syscall.hpp>
#include <utils/impl/assert_extra.hpp>
#include <utils/statistics/thread_statistics.hpp>

#include "child_process_map.hpp"

USERVER_NAMESPACE_BEGIN

namespace engine::ev {
namespace {

// We approach libev/OS timer resolution here
constexpr std::chrono::milliseconds kPeriodicEventsDriverInterval{1};

// There is a periodic timer in ev-thread with 1ms interval that processes
// starts/restarts/stops, thus we don't need to explicitly call ev_async_send
// for every operation. However there are some timers with resolution lower
// than 1ms, and for such presumably rare occasions we still want to notify
// ev-loop explicitly.
const auto kEventImmediateSetupThreshold =
    kPeriodicEventsDriverInterval +
    utils::datetime::SteadyCoarseClock::resolution();

std::atomic_flag& GetEvDefaultLoopFlag() {
  static std::atomic_flag ev_default_loop_flag ATOMIC_FLAG_INIT;
  return ev_default_loop_flag;
}

void AcquireEvDefaultLoop(const std::string& thread_name) {
  auto& ev_default_loop_flag = GetEvDefaultLoopFlag();
  if (ev_default_loop_flag.test_and_set())
    throw std::runtime_error(
        "Trying to use more than one ev_default_loop, thread_name=" +
        thread_name);
  LOG_DEBUG() << "Acquire ev_default_loop for thread_name=" << thread_name;
}

void ReleaseEvDefaultLoop() {
  auto& ev_default_loop_flag = GetEvDefaultLoopFlag();
  LOG_DEBUG() << "Release ev_default_loop";
  ev_default_loop_flag.clear();
}

constexpr std::chrono::milliseconds kCpuStatsCollectInterval{1000};
constexpr std::size_t kCpuStatsThrottle{16};

}  // namespace

Thread::Thread(const std::string& thread_name,
               RegisterEventMode register_event_mode)
    : Thread(thread_name, false, register_event_mode) {}

Thread::Thread(const std::string& thread_name, UseDefaultEvLoop,
               RegisterEventMode register_event_mode)
    : Thread(thread_name, true, register_event_mode) {}

Thread::Thread(const std::string& thread_name, bool use_ev_default_loop,
               RegisterEventMode register_event_mode)
    : use_ev_default_loop_(use_ev_default_loop),
      register_event_mode_(register_event_mode),
      loop_(nullptr),
      lock_(loop_mutex_, std::defer_lock),
      name_{thread_name},
      cpu_stats_storage_{kCpuStatsCollectInterval, kCpuStatsThrottle},
      is_running_(false) {
  if (use_ev_default_loop_) AcquireEvDefaultLoop(name_);
  Start();
}

Thread::~Thread() {
  StopEventLoop();
  if (use_ev_default_loop_) ReleaseEvDefaultLoop();
  UASSERT(loop_ == nullptr);
}

void Thread::RunInEvLoopAsync(AsyncPayloadBase& payload) noexcept {
  RegisterInEvLoop(payload);

  if (!IsInEvThread()) {
    ev_async_send(loop_, &watch_update_);
  }
}

void Thread::RunInEvLoopDeferred(AsyncPayloadBase& payload,
                                 Deadline deadline) noexcept {
  switch (register_event_mode_) {
    case RegisterEventMode::kImmediate: {
      RunInEvLoopAsync(payload);
      return;
    }
    case RegisterEventMode::kDeferred: {
      if (deadline.IsReachable() &&
          deadline.TimeLeftApprox() < kEventImmediateSetupThreshold) {
        RunInEvLoopAsync(payload);
      } else {
        RegisterInEvLoop(payload);
      }
      return;
    }
  }
}

void Thread::RegisterInEvLoop(AsyncPayloadBase& payload) {
  if (IsInEvThread()) {
    payload.PerformAndRelease();
    return;
  }

  func_queue_.Push(payload);
}

bool Thread::IsInEvThread() const {
  return (std::this_thread::get_id() == thread_.get_id());
}

std::uint8_t Thread::GetCurrentLoadPercent() const {
  return cpu_stats_storage_.GetCurrentLoadPercent();
}

const std::string& Thread::GetName() const { return name_; }

void Thread::Start() {
  loop_ = use_ev_default_loop_ ? ev_default_loop(EVFLAG_AUTO)
                               : ev_loop_new(EVFLAG_AUTO);
  UASSERT(loop_);
  ev_set_userdata(loop_, this);
  ev_set_loop_release_cb(loop_, Release, Acquire);

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
  ev_async_init(&watch_update_, UpdateLoopWatcher);
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
  ev_set_priority(&watch_update_, 1);
  ev_async_start(loop_, &watch_update_);

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
  ev_async_init(&watch_break_, BreakLoopWatcher);
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
  ev_set_priority(&watch_break_, EV_MAXPRI);
  ev_async_start(loop_, &watch_break_);

  using LibEvDuration = std::chrono::duration<double>;
  if (register_event_mode_ == RegisterEventMode::kDeferred) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
    ev_timer_init(
        &timers_driver_, UpdateTimersWatcher, 0.0,
        std::chrono::duration_cast<LibEvDuration>(kPeriodicEventsDriverInterval)
            .count());
    ev_timer_start(loop_, &timers_driver_);
  } else {
    // We set up a noop high-interval timer here to wake up the loop: that
    // helps to avoid cpu-stats stall with outdated values if we become idle
    // after a burst. No point in doing so if we already have timers_driver.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
    ev_timer_init(
        &stats_timer_, UpdateTimersWatcher, 0.0,
        std::chrono::duration_cast<LibEvDuration>(kCpuStatsCollectInterval)
            .count());
    ev_timer_start(loop_, &stats_timer_);
  }

  if (use_ev_default_loop_) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
    ev_child_init(&watch_child_, ChildWatcher, 0, 0);
    ev_child_start(loop_, &watch_child_);
  }

  is_running_ = true;
  thread_ = std::thread([this] {
    utils::SetCurrentThreadName(name_);
    RunEvLoop();
  });
}

void Thread::StopEventLoop() {
  ev_async_send(loop_, &watch_break_);
  if (thread_.joinable()) thread_.join();

  if (func_queue_.TryPop()) {
    utils::impl::AbortWithStacktrace("Some work was enqueued on a dead Thread");
  }

  if (!use_ev_default_loop_) ev_loop_destroy(loop_);
  loop_ = nullptr;
}

void Thread::RunEvLoop() {
  while (is_running_) {
    AcquireImpl();
    ev_run(loop_, EVRUN_ONCE);
    UpdateLoopWatcherImpl();
    cpu_stats_storage_.Collect();
    ReleaseImpl();
  }

  ev_async_stop(loop_, &watch_update_);
  ev_async_stop(loop_, &watch_break_);
  if (register_event_mode_ == RegisterEventMode::kDeferred) {
    ev_timer_stop(loop_, &timers_driver_);
  } else {
    ev_timer_stop(loop_, &stats_timer_);
  }
  if (use_ev_default_loop_) ev_child_stop(loop_, &watch_child_);
}

void Thread::UpdateLoopWatcher(struct ev_loop* loop, ev_async*, int) noexcept {
  auto* ev_thread = static_cast<Thread*>(ev_userdata(loop));
  UASSERT(ev_thread != nullptr);
  ev_thread->UpdateLoopWatcherImpl();
}

void Thread::UpdateTimersWatcher(struct ev_loop* loop, ev_timer*,
                                 int) noexcept {
  auto* ev_thread = static_cast<Thread*>(ev_userdata(loop));
  UASSERT(ev_thread != nullptr);
  ev_thread->UpdateLoopWatcherImpl();
}

void Thread::UpdateLoopWatcherImpl() {
  while (AsyncPayloadBase* payload = func_queue_.TryPop()) {
    LOG_TRACE() << "Thread::UpdateLoopWatcherImpl(), "
                << compiler::GetTypeName(typeid(*payload));
    try {
      payload->PerformAndRelease();
    } catch (const std::exception& ex) {
      LOG_WARNING() << "exception in async thread func: " << ex;
    }
  }
}

void Thread::BreakLoopWatcher(struct ev_loop* loop, ev_async*, int) noexcept {
  auto* ev_thread = static_cast<Thread*>(ev_userdata(loop));
  UASSERT(ev_thread != nullptr);
  ev_thread->BreakLoopWatcherImpl();
}

void Thread::BreakLoopWatcherImpl() {
  is_running_ = false;
  UpdateLoopWatcherImpl();
  ev_break(loop_, EVBREAK_ALL);
}

void Thread::ChildWatcher(struct ev_loop*, ev_child* w, int) noexcept {
  try {
    ChildWatcherImpl(w);
  } catch (const std::exception& ex) {
    LOG_ERROR() << "Exception in ChildWatcherImpl(): " << ex;
  }
}

void Thread::ChildWatcherImpl(ev_child* w) {
  auto* child_process_info = ChildProcessMapGetOptional(w->rpid);
  UASSERT(child_process_info);
  if (!child_process_info) {
    LOG_ERROR()
        << "Got signal for thread with pid=" << w->rpid
        << ", status=" << w->rstatus
        << ", but thread with this pid was not found in child_process_map";
    return;
  }

  auto process_status = subprocess::ChildProcessStatus{
      w->rstatus,
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::steady_clock::now() - child_process_info->start_time)};
  if (process_status.IsExited() || process_status.IsSignaled()) {
    LOG_INFO() << "Child process with pid=" << w->rpid << " was "
               << (process_status.IsExited() ? "exited normally"
                                             : "terminated by a signal");
    child_process_info->status_promise.set_value(std::move(process_status));
    ChildProcessMapErase(w->rpid);
  } else {
    if (WIFSTOPPED(w->rstatus)) {
      LOG_WARNING() << "Child process with pid=" << w->rpid
                    << " was stopped with signal=" << WSTOPSIG(w->rstatus);
    } else {
      const bool continued = WIFCONTINUED(w->rstatus);
      if (continued) {
        LOG_WARNING() << "Child process with pid=" << w->rpid << " was resumed";
      } else {
        LOG_WARNING()
            << "Child process with pid=" << w->rpid
            << " was notified in ChildWatcher with unknown reason (w->rstatus="
            << w->rstatus << ')';
      }
    }
  }
}

void Thread::Acquire(struct ev_loop* loop) noexcept {
  auto* ev_thread = static_cast<Thread*>(ev_userdata(loop));
  UASSERT(ev_thread != nullptr);
  ev_thread->AcquireImpl();
}

void Thread::Release(struct ev_loop* loop) noexcept {
  auto* ev_thread = static_cast<Thread*>(ev_userdata(loop));
  UASSERT(ev_thread != nullptr);
  ev_thread->ReleaseImpl();
}

void Thread::AcquireImpl() noexcept { lock_.lock(); }
void Thread::ReleaseImpl() noexcept { lock_.unlock(); }

}  // namespace engine::ev

USERVER_NAMESPACE_END
