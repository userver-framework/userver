#pragma once

#include <chrono>

#include <userver/engine/future.hpp>
#include <userver/engine/subprocess/child_process_status.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::ev {

struct ChildProcessMapValue {
  explicit ChildProcessMapValue(
      engine::Promise<subprocess::ChildProcessStatus> status_promise)
      : start_time(std::chrono::steady_clock::now()),
        status_promise(std::move(status_promise)) {}

  std::chrono::steady_clock::time_point start_time;
  engine::Promise<subprocess::ChildProcessStatus> status_promise;
};

// All ChildProcessMap* methods should be called from ev_default_loop's thread
// only.
ChildProcessMapValue* ChildProcessMapGetOptional(int pid);

void ChildProcessMapErase(int pid);

std::pair<ChildProcessMapValue*, bool> ChildProcessMapSet(
    int pid, ChildProcessMapValue&& value);

}  // namespace engine::ev

USERVER_NAMESPACE_END
