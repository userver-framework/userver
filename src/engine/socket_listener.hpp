#pragma once

#include <functional>
#include <mutex>

#include <ev.h>

#include <engine/condition_variable.hpp>
#include <engine/ev/thread_control.hpp>

#include "watcher.hpp"

namespace engine {

class SocketListener {
 public:
  enum class Result { kOk, kAgain, kError };

  enum class ListenMode { kRead, kWrite };

  struct DeferStart {};

  using ListenFunc = std::function<Result(int fd)>;
  using OnStopFunc = std::function<void()>;

  SocketListener(const ev::ThreadControl& thread_control,
                 TaskProcessor& task_processor, int fd, ListenMode mode,
                 ListenFunc&& listen_func, OnStopFunc&& on_stop_func);
  SocketListener(const ev::ThreadControl& thread_control,
                 TaskProcessor& task_processor, int fd, ListenMode mode,
                 ListenFunc&& listen_func, OnStopFunc&& on_stop_func,
                 DeferStart);
  ~SocketListener();

  void Start();
  void Notify();
  void Stop();
  bool IsRunning() const;
  int Fd() const { return fd_; }

 private:
  struct WatcherListenState {
    std::mutex mutex;
    bool is_enabled = false;
  };

  void StopAsync();
  void InitWatchers();
  static void WatcherListen(struct ev_loop*, ev_io* w, int);
  void WatcherListenImpl();
  void WatcherListenResume();
  void StartListenTask(TaskProcessor& task_processor);
  void SetWatcherListenIsEnabled(bool is_enabled);

  TaskProcessor& task_processor_;
  int fd_;
  ListenMode mode_;
  ListenFunc listen_func_;
  OnStopFunc on_stop_func_;

  std::shared_ptr<WatcherListenState> watcher_listen_resume_state_;
  Watcher<ev_io> watcher_listen_;

  mutable std::mutex listen_cv_mutex_;
  ConditionVariable listen_cv_;
  bool is_running_ = false;
  bool is_notified_ = false;

  std::mutex stop_mutex_;
  ConditionVariable listen_finished_cv_;
  bool is_listen_finished_ = true;
};

}  // namespace engine
