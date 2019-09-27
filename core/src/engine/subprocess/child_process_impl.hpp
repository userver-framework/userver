#pragma once

#include <chrono>

#include <engine/future.hpp>

#include <engine/subprocess/child_process_status.hpp>

namespace engine {
namespace subprocess {

class ChildProcessImpl {
 public:
  ChildProcessImpl(int pid, Future<ChildProcessStatus>&& status_future);

  int GetPid() const { return pid_; }

  void Wait();
  bool WaitUntil(const std::chrono::steady_clock::time_point& until);
  ChildProcessStatus Get();
  void SendSignal(int signum);

 private:
  int pid_;
  Future<ChildProcessStatus> status_future_;
};

}  // namespace subprocess
}  // namespace engine
