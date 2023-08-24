#include <userver/testsuite/tasks.hpp>

#include <atomic>

#include <fmt/core.h>

#include <userver/components/component_context.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/logging/log.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/uuid4.hpp>
#include <utils/impl/assert_extra.hpp>

USERVER_NAMESPACE_BEGIN

namespace testsuite {
namespace {

class FlagClearer {
 public:
  FlagClearer(std::atomic<bool>& flag) : flag_(flag) {}
  ~FlagClearer() { flag_.store(false); }

 private:
  std::atomic<bool>& flag_;
};

}  // namespace

TestsuiteTasks::TestsuiteTasks(bool is_enabled) : is_enabled_(is_enabled) {}

TestsuiteTasks::~TestsuiteTasks() { CheckNoRunningTasks(); }

void TestsuiteTasks::RegisterTask(const std::string& name,
                                  TaskCallback callback) {
  auto locked_tasks = tasks_.Lock();
  auto it = locked_tasks->find(name);
  if (it != locked_tasks->end()) {
    LOG_ERROR() << "Testsuite task '" << name << "' already registered";
    throw std::runtime_error(
        fmt::format("Testsuite task '{}' already registered", name));
  }
  LOG_INFO() << "Testsuite task " << name << " registered";

  auto entry = std::make_shared<Entry>();
  entry->callback = std::move(callback);
  locked_tasks->emplace(name, std::move(entry));
}

void TestsuiteTasks::UnregisterTask(const std::string& name) {
  auto locked_tasks = tasks_.Lock();
  auto it = locked_tasks->find(name);

  if (it == locked_tasks->end()) {
    LOG_ERROR() << "Testsuite task " << name << " is not registered";
    throw std::runtime_error(
        fmt::format("Testsuite task '{}' is not registered", name));
  }
  LOG_INFO() << "Testsuite task " << name << " unregistered";

  locked_tasks->erase(it);
}

void TestsuiteTasks::RunTask(const std::string& name) {
  auto entry = GetEntryFor(name);

  if (entry->running_flag.exchange(true)) {
    throw TaskAlreadyRunning(fmt::format("Task '{}' is already running", name));
  }

  FlagClearer clearer(entry->running_flag);
  entry->callback();
}

std::string TestsuiteTasks::SpawnTask(const std::string& name) {
  std::string task_id = utils::generators::GenerateUuid();
  auto entry = GetEntryFor(name);

  if (entry->running_flag.exchange(true)) {
    throw TaskAlreadyRunning(
        fmt::format("Testsuite task '{}' is already running", name));
  }

  auto spawned = std::make_shared<SpawnedTask>();
  spawned->name = name;
  spawned->task = utils::Async("testsuite-task/" + name, [entry] {
    FlagClearer clearer(entry->running_flag);
    entry->callback();
  });

  auto locked_spawned = spawned_.Lock();
  locked_spawned->emplace(task_id, spawned);
  return task_id;
}

void TestsuiteTasks::StopSpawnedTask(const std::string& task_id) {
  auto spawned = GetSpawnedFor(task_id);

  if (spawned->busy_flag.exchange(true)) {
    throw TaskAlreadyRunning("Spawned testsuite task is locked");
  }

  FlagClearer clearer(spawned->busy_flag);
  bool is_finished = spawned->task.IsFinished();

  if (is_finished) {
    LOG_INFO() << "Testsuite task " << task_id << " is already finished";
  } else {
    LOG_INFO() << "Testsuite task " << task_id << " is running, cancelling";
    spawned->task.SyncCancel();
  }

  {
    auto locked = spawned_.Lock();
    locked->erase(task_id);
  }

  if (is_finished) spawned->task.Get();
}

std::shared_ptr<TestsuiteTasks::Entry> TestsuiteTasks::GetEntryFor(
    const std::string& name) {
  auto locked = tasks_.Lock();
  auto it = locked->find(name);
  if (it == locked->end()) {
    if (!is_enabled_) {
      throw TaskNotFound(
          fmt::format("Testsuite task '{}' not found probably due to "
                      "'testsuite-support.testsuite-tasks-enabled' is 'false' "
                      "in static config, ",
                      name));
    } else {
      throw TaskNotFound(fmt::format("Testsuite task '{}' not found", name));
    }
  }
  return it->second;
}

std::shared_ptr<TestsuiteTasks::SpawnedTask> TestsuiteTasks::GetSpawnedFor(
    const std::string& task_id) {
  auto locked = spawned_.Lock();
  auto it = locked->find(task_id);
  if (it == locked->end()) {
    throw TaskNotFound("Spawned testsuite task not found");
  }
  return it->second;
}

void TestsuiteTasks::CheckNoRunningTasks() noexcept {
  {
    auto locked_tasks = tasks_.Lock();
    for (const auto& it : *locked_tasks) {
      if (it.second->running_flag) {
        utils::impl::AbortWithStacktrace(
            fmt::format("Testsuite task '{}' is still running", it.first));
      }
    }
  }

  {
    auto locked = spawned_.Lock();
    for (const auto& it : *locked) {
      utils::impl::AbortWithStacktrace(fmt::format(
          "Spawned testsuite task '{}' is still running", it.second->name));
    }
  }
}

std::vector<std::string> TestsuiteTasks::GetTaskNames() const {
  std::vector<std::string> result;
  auto locked = tasks_.Lock();

  result.reserve(locked->size());
  for (const auto& it : *locked) {
    result.push_back(it.first);
  }
  return result;
}

TestsuiteTasks& GetTestsuiteTasks(
    const components::ComponentContext& component_context) {
  auto& testsuite_support =
      component_context.FindComponent<components::TestsuiteSupport>();
  return testsuite_support.GetTestsuiteTasks();
}

}  // namespace testsuite

USERVER_NAMESPACE_END
