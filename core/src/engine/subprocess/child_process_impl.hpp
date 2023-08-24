#pragma once

#include <chrono>

#include <userver/engine/deadline.hpp>
#include <userver/engine/future.hpp>
#include <userver/engine/subprocess/child_process_status.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::subprocess {

class ChildProcessImpl {
 public:
  ChildProcessImpl(int pid, Future<ChildProcessStatus>&& status_future);

  int GetPid() const { return pid_; }

  void WaitNonCancellable();

  [[nodiscard]] bool WaitUntil(Deadline deadline);

  ChildProcessStatus Get();

  void SendSignal(int signum);

 private:
  int pid_;
  Future<ChildProcessStatus> status_future_;
};

}  // namespace engine::subprocess

USERVER_NAMESPACE_END
