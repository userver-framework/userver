#include "thread.hpp"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include <engine/task/cancel.hpp>
#include <logging/log.hpp>
#include <utils/assert.hpp>
#include <utils/thread_name.hpp>
#include <utils/userver_experiment.hpp>

namespace engine {
namespace ev {
namespace {

const size_t kInitFuncQueueCapacity = 64;

}  // namespace

Thread::Thread(const std::string& thread_name)
    :  // NOLINTNEXTLINE(clang-analyzer-core.uninitialized.Assign)
      func_queue_(kInitFuncQueueCapacity),
      loop_(nullptr),
      lock_(loop_mutex_, std::defer_lock),
      is_running_(false) {
  Start();
  utils::SetThreadName(thread_, thread_name);
}

Thread::~Thread() {
  StopEventLoop();
  UASSERT(loop_ == nullptr);
}

void Thread::AsyncStartUnsafe(ev_async& w) { ev_async_start(GetEvLoop(), &w); }

void Thread::AsyncStart(ev_async& w) {
  SafeEvCall([this, &w]() { AsyncStartUnsafe(w); });
}

void Thread::AsyncStopUnsafe(ev_async& w) { ev_async_stop(GetEvLoop(), &w); }

void Thread::AsyncStop(ev_async& w) {
  SafeEvCall([this, &w]() { AsyncStopUnsafe(w); });
}

void Thread::TimerStartUnsafe(ev_timer& w) {
  ev_now_update(GetEvLoop());
  ev_timer_start(GetEvLoop(), &w);
}

void Thread::TimerStart(ev_timer& w) {
  SafeEvCall([this, &w]() { TimerStartUnsafe(w); });
}

void Thread::TimerAgainUnsafe(ev_timer& w) {
  ev_now_update(GetEvLoop());
  ev_timer_again(GetEvLoop(), &w);
}

void Thread::TimerAgain(ev_timer& w) {
  SafeEvCall([this, &w]() { TimerAgainUnsafe(w); });
}

void Thread::TimerStopUnsafe(ev_timer& w) { ev_timer_stop(GetEvLoop(), &w); }

void Thread::TimerStop(ev_timer& w) {
  SafeEvCall([this, &w]() { TimerStopUnsafe(w); });
}

void Thread::IoStartUnsafe(ev_io& w) { ev_io_start(GetEvLoop(), &w); }

void Thread::IoStart(ev_io& w) {
  SafeEvCall([this, &w]() { IoStartUnsafe(w); });
}

void Thread::IoStopUnsafe(ev_io& w) { ev_io_stop(GetEvLoop(), &w); }

void Thread::IoStop(ev_io& w) {
  SafeEvCall([this, &w]() { IoStopUnsafe(w); });
}

void Thread::IdleStartUnsafe(ev_idle& w) { ev_idle_start(GetEvLoop(), &w); }

void Thread::IdleStart(ev_idle& w) {
  SafeEvCall([this, &w]() { IdleStartUnsafe(w); });
}

void Thread::IdleStopUnsafe(ev_idle& w) { ev_idle_stop(GetEvLoop(), &w); }

void Thread::IdleStop(ev_idle& w) {
  SafeEvCall([this, &w]() { IdleStopUnsafe(w); });
}

void Thread::RunInEvLoopAsync(std::function<void()>&& func) {
  if (IsInEvThread()) {
    func();
    return;
  }

  auto func_ptr = std::make_unique<std::function<void()>>(std::move(func));
  if (!func_queue_.push(func_ptr.release())) {
    LOG_ERROR() << "can't push func to queue";
    throw std::runtime_error("can't push func to queue");
  }
  ev_async_send(loop_, &watch_update_);
}

bool Thread::IsInEvThread() const {
  return (std::this_thread::get_id() == thread_.get_id());
}

template <typename Func>
void Thread::SafeEvCall(const Func& func) {
  if (IsInEvThread()) {
    func();
    return;
  }

  {
    std::lock_guard<std::mutex> lock(loop_mutex_);
    func();
  }
  ev_async_send(loop_, &watch_update_);
}

void Thread::Start() {
  loop_ = ev_loop_new(EVFLAG_AUTO);
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

  is_running_ = true;
  thread_ = std::thread([this] { RunEvLoop(); });
}

void Thread::StopEventLoop() {
  ev_async_send(loop_, &watch_break_);
  if (thread_.joinable()) thread_.join();
  ev_loop_destroy(loop_);
  loop_ = nullptr;
}

void Thread::RunEvLoop() {
  while (is_running_) {
    AcquireImpl();
    ev_run(loop_, EVRUN_ONCE);
    ReleaseImpl();
  }

  ev_async_stop(loop_, &watch_update_);
  ev_async_stop(loop_, &watch_break_);
}

void Thread::UpdateLoopWatcher(struct ev_loop* loop, ev_async*, int) try {
  auto* ev_thread = static_cast<Thread*>(ev_userdata(loop));
  UASSERT(ev_thread != nullptr);
  ev_thread->UpdateLoopWatcherImpl();
} catch (...) {
  if (utils::IsUserverExperimentEnabled(
          utils::UserverExperiment::kTaxicommon1479)) {
    std::cerr << "Uncaught exception in " << __PRETTY_FUNCTION__;
    abort();
  }
  throw;
}

void Thread::UpdateLoopWatcherImpl() {
  LOG_TRACE() << "Thread::UpdateLoopWatcherImpl() "
                 "func_queue_.empty()="
              << func_queue_.empty();

  std::function<void()>* pfunc;
  while (func_queue_.pop(pfunc)) {
    LOG_TRACE() << "Thread::UpdateLoopWatcherImpl() (loop)";
    std::unique_ptr<std::function<void()>> func(pfunc);
    try {
      (*func)();
    } catch (const std::exception& ex) {
      LOG_WARNING() << "exception in async thread func: " << ex;
    }
  }
  LOG_TRACE() << "exit";
}

void Thread::BreakLoopWatcher(struct ev_loop* loop, ev_async*, int) try {
  auto* ev_thread = static_cast<Thread*>(ev_userdata(loop));
  UASSERT(ev_thread != nullptr);
  // NOLINTNEXTLINE(clang-analyzer-core.UndefinedBinaryOperatorResult)
  ev_thread->BreakLoopWatcherImpl();
} catch (...) {
  if (utils::IsUserverExperimentEnabled(
          utils::UserverExperiment::kTaxicommon1479)) {
    std::cerr << "Uncaught exception in " << __PRETTY_FUNCTION__;
    abort();
  }
  throw;
}

void Thread::BreakLoopWatcherImpl() {
  is_running_ = false;
  UpdateLoopWatcherImpl();
  ev_break(loop_, EVBREAK_ALL);
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

}  // namespace ev
}  // namespace engine
