#include <dist_lock/impl/helpers.hpp>

#include <engine/task/cancel.hpp>
#include <logging/log.hpp>

namespace dist_lock::impl {

bool GetTask(engine::TaskWithResult<void>& task, const std::string& name) {
  try {
    engine::TaskCancellationBlocker cancel_blocker;
    if (task.IsValid()) {
      task.Get();
      return true;
    }
  } catch (const engine::TaskCancelledException& e) {
    // Do nothing
  } catch (const std::exception& e) {
    LOG_ERROR() << "Unexpected exception on " << name << ".Get(): " << e;
  }
  return false;
}

std::string LockerName(const std::string& lock_name) {
  return "locker-" + lock_name;
}

std::string WatchdogName(const std::string& lock_name) {
  return "watchdog-" + lock_name;
}

std::string WorkerName(const std::string& lock_name) {
  return "lock-worker-" + lock_name;
}

}  // namespace dist_lock::impl
