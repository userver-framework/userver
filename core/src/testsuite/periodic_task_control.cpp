#include <userver/testsuite/periodic_task_control.hpp>

#include <stdexcept>

#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/periodic_task.hpp>

USERVER_NAMESPACE_BEGIN

namespace testsuite {

void PeriodicTaskControl::RegisterPeriodicTask(const std::string& name,
                                               utils::PeriodicTask& task) {
  UINVARIANT(!name.empty(),
             "RegisterInTestsuite called on an empty PeriodicTask. Call "
             "PeriodicTask::Start before RegisterInTestsuite.");
  auto periodic_tasks = periodic_tasks_.Lock();
  const auto [_, success] = periodic_tasks->emplace(name, task);
  UINVARIANT(success, "Periodic task name " + name + " already registered");
}

void PeriodicTaskControl::UnregisterPeriodicTask(const std::string& name,
                                                 utils::PeriodicTask& task) {
  auto periodic_tasks = periodic_tasks_.Lock();
  const auto it = periodic_tasks->find(name);

  UINVARIANT(it != periodic_tasks->end(),
             "Periodic task name " + name + " is not known");
  UINVARIANT(&it->second == &task,
             "Periodic task name " + name + " does not belong to the caller");

  periodic_tasks->erase(it);
}

bool PeriodicTaskControl::RunPeriodicTask(const std::string& name) {
  LOG_INFO() << "Executing periodic task " << name << " once";
  auto& task = FindPeriodicTask(name);
  const bool status = task.SynchronizeDebug(true);

  if (status) {
    LOG_INFO() << "Periodic task " << name << " completed successfully";
  } else {
    LOG_ERROR() << "Periodic task " << name << " failed";
  }
  return status;
}

void PeriodicTaskControl::SuspendPeriodicTasks(
    const std::unordered_set<std::string>& names) {
  const auto periodic_tasks = periodic_tasks_.Lock();

  for (const auto& [name, task] : *periodic_tasks) {
    if (names.count(name)) {
      task.SuspendDebug();
    } else {
      task.ResumeDebug();
    }
  }
}

utils::PeriodicTask& PeriodicTaskControl::FindPeriodicTask(
    const std::string& name) {
  const auto periodic_tasks = periodic_tasks_.Lock();
  const auto it = periodic_tasks->find(name);
  UINVARIANT(it != periodic_tasks->end(),
             "Periodic task name " + name + " is not known");
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

USERVER_NAMESPACE_END
