#include "thread.hpp"

#include "timer.hpp"

#include <logger/logger.hpp>

namespace engine {
namespace ev {
namespace {

const size_t kDefaultFuncQueueSize = 64;

}  // namespace

Thread::Thread(const std::string& thread_name)
    : func_ptr_(nullptr),
      func_queue_(kDefaultFuncQueueSize),
      loop_(nullptr),
      lock_(loop_mutex_, std::defer_lock),
      running_(false) {
  Run(thread_name);
}

Thread::~Thread() {
  StopEventLoop();
  assert(loop_ == nullptr);
}

void Thread::SetThreadName(const std::string& name) {
  pthread_setname_np(thread_.native_handle(), name.substr(0, 15).c_str());
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

void Thread::RunInEvLoopSync(const std::function<void()>& func) {
  if (CheckThread()) {
    func();
    return;
  }

  {
    std::lock_guard<std::mutex> lock(mutex_);
    func_ptr_ = &func;
    UpdateEvLoop();
  }
}

void Thread::RunInEvLoopAsync(std::function<void()>&& func) {
  if (CheckThread()) {
    func();
    return;
  }

  auto func_ptr = std::make_unique<std::function<void()>>(std::move(func));
  func_queue_.push(func_ptr.release());
  ev_async_send(loop_, &watch_update_);
}

template <typename Func>
void Thread::SafeEvCall(const Func& func) {
  if (CheckThread()) {
    func();
    return;
  }

  {
    std::lock_guard<std::mutex> lock(loop_mutex_);
    func();
  }
  ev_async_send(loop_, &watch_update_);
}

void Thread::Run(const std::string& thread_name) {
  loop_ = ev_loop_new(EVFLAG_AUTO);
  assert(loop_);
  ev_set_userdata(loop_, this);

  std::promise<void> thread_ready;
  running_ = true;
  thread_ = std::thread([this, &thread_ready]() {
    watch_update_.data = this;
    ev_async_init(&watch_update_, UpdateLoopWatcher);
    ev_set_priority(&watch_update_, 1);
    ev_async_start(loop_, &watch_update_);

    watch_break_.data = this;
    ev_async_init(&watch_break_, BreakLoopWatcher);
    ev_set_priority(&watch_break_, EV_MAXPRI);
    ev_async_start(loop_, &watch_break_);

    ev_set_loop_release_cb(loop_, Release, Acquire);

    thread_ready.set_value();
    RunEvLoop();
  });
  thread_ready.get_future().get();
  if (!thread_name.empty()) SetThreadName(thread_name);
}

void Thread::StopEventLoop() {
  ev_async_send(loop_, &watch_break_);
  if (thread_.joinable()) thread_.join();
  ev_loop_destroy(loop_);
  loop_ = nullptr;
}

void Thread::UpdateEvLoop() {
  if (CheckThread()) return;
  assert(!func_promise_);
  auto func_promise = std::make_unique<std::promise<void>>();
  auto func_future = func_promise->get_future();
  func_promise_ = std::move(func_promise);
  ev_async_send(loop_, &watch_update_);
  func_future.get();
}

bool Thread::CheckThread() const {
  return (std::this_thread::get_id() == thread_.get_id());
}

void Thread::RunEvLoop() {
  while (running_) {
    AcquireImpl();
    ev_run(loop_, EVRUN_ONCE);
    ReleaseImpl();
  }

  ev_async_stop(loop_, &watch_update_);
  ev_async_stop(loop_, &watch_break_);
}

void Thread::UpdateLoopWatcher(struct ev_loop*, ev_async* w, int) {
  Thread* ev_thread = static_cast<Thread*>(w->data);
  assert(ev_thread != nullptr);
  ev_thread->UpdateLoopWatcherImpl();
}

void Thread::UpdateLoopWatcherImpl() {
  if (func_promise_) {
    assert(func_ptr_);
    std::exception_ptr ex_ptr;
    try {
      (*func_ptr_)();
    } catch (const std::exception& ex) {
      ex_ptr = std::current_exception();
    }
    func_ptr_ = nullptr;
    std::unique_ptr<std::promise<void>> func_promise = std::move(func_promise_);
    func_promise_.reset();
    try {
      if (ex_ptr)
        func_promise->set_exception(ex_ptr);
      else
        func_promise->set_value();
    } catch (const std::exception& ex) {
      LOG_ERROR() << "can't set value or exception: " << ex.what();
    }
  }

  std::function<void()>* pfunc;
  while (func_queue_.pop(pfunc)) {
    std::unique_ptr<std::function<void()>> func(pfunc);
    try {
      (*func)();
    } catch (const std::exception& ex) {
      LOG_WARNING() << "exception in async thread func: " << ex.what();
    }
  }
}

void Thread::BreakLoopWatcher(struct ev_loop*, ev_async* w, int) {
  Thread* ev_thread = static_cast<Thread*>(w->data);
  assert(ev_thread != nullptr);
  ev_thread->BreakLoopWatcherImpl();
}

void Thread::BreakLoopWatcherImpl() {
  running_ = false;
  UpdateLoopWatcherImpl();
  ev_break(loop_, EVBREAK_ALL);
}

void Thread::Release(struct ev_loop* loop) {
  Thread* ev_thread = static_cast<Thread*>(ev_userdata(loop));
  assert(ev_thread != nullptr);
  ev_thread->ReleaseImpl();
}

void Thread::ReleaseImpl() { lock_.unlock(); }

void Thread::Acquire(struct ev_loop* loop) {
  Thread* ev_thread = static_cast<Thread*>(ev_userdata(loop));
  assert(ev_thread != nullptr);
  ev_thread->AcquireImpl();
}

void Thread::AcquireImpl() { lock_.lock(); }

}  // namespace ev
}  // namespace engine
