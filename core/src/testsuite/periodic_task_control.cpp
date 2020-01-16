#include <testsuite/periodic_task_control.hpp>

#include <stdexcept>
#include <unordered_set>

#include <logging/log.hpp>
#include <utils/assert.hpp>
#include <utils/periodic_task.hpp>

namespace testsuite {

void PeriodicTaskControl::RegisterPeriodicTask(const std::string& name,
                                               utils::PeriodicTask& task) {
  std::lock_guard lock(mutex_);

  if (!periodic_tasks_.emplace(name, task).second) {
    auto error_msg = "Periodic task name " + name + " already registred";
    UASSERT_MSG(false, error_msg);
    throw std::runtime_error(error_msg);
  }
}

void PeriodicTaskControl::UnregisterPeriodicTask(const std::string& name,
                                                 utils::PeriodicTask& task) {
  std::lock_guard lock(mutex_);

  auto it = periodic_tasks_.find(name);
  if (it == periodic_tasks_.end()) {
    auto error_msg = "Periodic task name " + name + " is not known";
    UASSERT_MSG(false, error_msg);
    LOG_ERROR() << error_msg;
  } else {
    if (&it->second != &task) {
      auto error_msg =
          "Periodic task name " + name + " does not belong to the caller";
      UASSERT_MSG(false, error_msg);
      LOG_ERROR() << error_msg;
    }
    periodic_tasks_.erase(name);
  }
}

bool PeriodicTaskControl::RunPeriodicTask(const std::string& name) {
  auto& task = FindPeriodicTask(name);

  static engine::Mutex run_task_mutex;
  std::lock_guard lock(run_task_mutex);
  return task.SynchronizeDebug(true);
}

void PeriodicTaskControl::SuspendPeriodicTasks(
    const std::vector<std::string>& names) {
  std::unordered_set<std::string> to_suspend;
  to_suspend.insert(names.cbegin(), names.cend());

  for (const auto& entry : periodic_tasks_) {
    const auto& name = entry.first;
    auto& task = entry.second;
    if (to_suspend.count(name)) {
      task.SuspendDebug();
    } else {
      task.ResumeDebug();
    }
  }
}

utils::PeriodicTask& PeriodicTaskControl::FindPeriodicTask(
    const std::string& name) {
  std::lock_guard lock(mutex_);

  auto it = periodic_tasks_.find(name);
  if (it == periodic_tasks_.end()) {
    throw std::runtime_error("Periodic task name " + name + " is not known");
  }
  return it->second;
}

PeriodicTaskRegistrationHolder::PeriodicTaskRegistrationHolder(
    PeriodicTaskControl& periodic_task_control, std::string name,
    utils::PeriodicTask& task)
    : periodic_task_control_(periodic_task_control),
      name_(std::move(name)),
      task_(task) {
  periodic_task_control_.RegisterPeriodicTask(name_, task_);
}

PeriodicTaskRegistrationHolder::~PeriodicTaskRegistrationHolder() {
  periodic_task_control_.UnregisterPeriodicTask(name_, task_);
}

}  // namespace testsuite
