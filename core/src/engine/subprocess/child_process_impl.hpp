#pragma once

#include <chrono>

#include <engine/blocking_future.hpp>
#include <userver/engine/subprocess/child_process_status.hpp>

namespace engine::subprocess {

class ChildProcessImpl {
 public:
  ChildProcessImpl(int pid,
                   impl::BlockingFuture<ChildProcessStatus>&& status_future);

  int GetPid() const { return pid_; }

  void Wait();
  bool WaitUntil(const std::chrono::steady_clock::time_point& until);
  ChildProcessStatus Get();
  void SendSignal(int signum);

 private:
  int pid_;
  impl::BlockingFuture<ChildProcessStatus> status_future_;
};

}  // namespace engine::subprocess
