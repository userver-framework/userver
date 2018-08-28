#include "socket_listener.hpp"

#include <sys/socket.h>

#include <cassert>

#include <engine/async.hpp>
#include <logging/log.hpp>

namespace engine {

SocketListener::SocketListener(const ev::ThreadControl& thread_control,
                               TaskProcessor& task_processor, int fd,
                               ListenMode mode, ListenFunc&& listen_func,
                               OnStopFunc&& on_stop_func)
    : SocketListener(thread_control, task_processor, fd, mode,
                     std::move(listen_func), std::move(on_stop_func),
                     DeferStart{}) {
  Start();
}

SocketListener::SocketListener(const ev::ThreadControl& thread_control,
                               TaskProcessor& task_processor, int fd,
                               ListenMode mode, ListenFunc&& listen_func,
                               OnStopFunc&& on_stop_func, DeferStart)
    : task_processor_(task_processor),
      fd_(fd),
      mode_(mode),
      listen_func_(std::move(listen_func)),
      on_stop_func_(std::move(on_stop_func)),
      watcher_listen_resume_state_(std::make_shared<WatcherListenState>()),
      watcher_listen_(thread_control, this) {
  assert(listen_func_);
  InitWatchers();
}

SocketListener::~SocketListener() { Stop(); }

void SocketListener::Start() {
  StartListenTask(task_processor_);
  SetWatcherListenIsEnabled(true);
  if (mode_ == ListenMode::kRead) watcher_listen_.Start();
}

void SocketListener::Notify() {
  {
    std::lock_guard<Mutex> lock(listen_cv_mutex_);
    is_notified_ = true;
  }
  listen_cv_.NotifyAll();
}

void SocketListener::Stop() {
  SetWatcherListenIsEnabled(false);
  watcher_listen_.Stop();
  StopAsync();
  listen_task_.Wait();
}

bool SocketListener::IsRunning() const {
  std::lock_guard<Mutex> lock(listen_cv_mutex_);
  return is_running_;
}

void SocketListener::StopAsync() {
  {
    std::lock_guard<Mutex> lock(listen_cv_mutex_);
    if (!is_running_) return;
    is_running_ = false;
  }
  listen_cv_.NotifyAll();
}

void SocketListener::InitWatchers() {
  int ev_mode = 0;
  switch (mode_) {
    case ListenMode::kRead:
      ev_mode = EV_READ;
      break;
    case ListenMode::kWrite:
      ev_mode = EV_WRITE;
      break;
  }
  watcher_listen_.Init(WatcherListen, fd_, ev_mode);
}

void SocketListener::WatcherListen(struct ev_loop*, ev_io* w, int) {
  auto socket_listener = static_cast<SocketListener*>(w->data);
  assert(socket_listener != nullptr);
  socket_listener->WatcherListenImpl();
}

void SocketListener::WatcherListenImpl() {
  watcher_listen_.Stop();

  engine::CriticalAsync(task_processor_,
                        [self = shared_from_this()] { self->Notify(); })
      .Detach();
}

void SocketListener::WatcherListenResume() {
  watcher_listen_.RunInEvLoopAsync(
      [ state = watcher_listen_resume_state_, this ]() {
        std::lock_guard<std::mutex> lock(state->mutex);
        if (!state->is_enabled) return;
        watcher_listen_.Start();
      });
}

void SocketListener::StartListenTask(TaskProcessor& task_processor) {
  is_running_ = true;
  listen_task_ = CriticalAsync(task_processor, [this]() {
    for (;;) {
      {
        std::unique_lock<Mutex> lock(listen_cv_mutex_);
        listen_cv_.Wait(lock,
                        [this]() { return !is_running_ || is_notified_; });
        if (!is_running_) break;
        assert(is_notified_);
        is_notified_ = false;
      }
      Result res = Result::kError;
      try {
        res = listen_func_(fd_);
        if (res == Result::kError) StopAsync();
      } catch (const std::exception& ex) {
        LOG_ERROR() << "exception in listen_func: " << ex.what();
        StopAsync();
      }
      if (res != Result::kError &&
          (mode_ == ListenMode::kRead ||
           (mode_ == ListenMode::kWrite && res == Result::kAgain))) {
        try {
          WatcherListenResume();
        } catch (const std::exception& ex) {
          LOG_ERROR() << "can't resume watcher_listen: " << ex.what();
          StopAsync();
        }
      }
    }

    try {
      if (on_stop_func_) on_stop_func_();
    } catch (const std::exception& ex) {
      LOG_ERROR() << "exception in on_stop_func: " << ex.what();
    }
  });
}

void SocketListener::SetWatcherListenIsEnabled(bool is_enabled) {
  std::lock_guard<std::mutex> lock(watcher_listen_resume_state_->mutex);
  watcher_listen_resume_state_->is_enabled = is_enabled;
}

}  // namespace engine
