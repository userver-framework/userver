#pragma once

/// @file userver/testsuite/tasks.hpp
/// @brief @copybrief testsuite::TestsuiteTasks

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include <userver/components/component_fwd.hpp>
#include <userver/concurrent/variable.hpp>
#include <userver/engine/task/task_with_result.hpp>

USERVER_NAMESPACE_BEGIN

namespace testsuite {

class TaskNotFound final : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

class TaskAlreadyRunning final : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

// clang-format off

/// @brief Testsuite tasks support
///
/// A testsuite task is a function that can be called from testsuite by its name.
/// Only one task with the same name can be run simultaneously.
///

// clang-format on
class TestsuiteTasks final {
 public:
  /// Task function type
  using TaskCallback = std::function<void()>;

  explicit TestsuiteTasks(bool is_enabled);
  ~TestsuiteTasks();

  /// @brief Are testsuite tasks enabled or not
  ///
  /// It is recommended to register tasks only for testsuite environment. Use
  /// this method to test weather or not you are running under testsuite:
  /// @code
  /// if (testsuite_tasks.IsEnabled()) {
  ///   // testsuite code goes here...
  /// } else {
  ///   // production code goes here...
  /// }
  /// @endcode
  bool IsEnabled() const { return is_enabled_; }

  /// @brief Register new task
  /// @param name Task name
  /// @param callback Testsuite task callback function
  /// @throws std::runtime_error When task with `name` is already registered.
  void RegisterTask(const std::string& name, TaskCallback callback);

  /// @brief Unregister task
  /// @param name Task name
  /// @throws std::runtime_error When task `name` is not known.
  void UnregisterTask(const std::string& name);

  /// @brief Run task by name and waits for it to finish
  /// @param name Task name
  /// @throws TaskAlreadyRunning When task `name` is already running.
  void RunTask(const std::string& name);

  /// @brief Asynchronously starts the task
  /// @param name Task name
  /// @returns Task id
  /// @throws TaskAlreadyRunning When task `name` is already running.
  std::string SpawnTask(const std::string& name);

  /// @brief Stop previously spawned task
  /// @param task_id Task id returned by `SpawnTask()`
  /// @throws TaskNotFound When task `task_id` is not known.
  void StopSpawnedTask(const std::string& task_id);

  /// @brief Checks that there are no running tasks left.
  ///
  /// Must be called on service shutdown. Aborts process when there are tasks
  /// running.
  void CheckNoRunningTasks() noexcept;

  /// @brief Returns list of registered task names
  std::vector<std::string> GetTaskNames() const;

 private:
  struct Entry {
    std::atomic<bool> running_flag{false};
    TaskCallback callback;
  };

  struct SpawnedTask {
    std::atomic<bool> busy_flag{false};
    std::string name;
    engine::TaskWithResult<void> task;
  };

  std::shared_ptr<Entry> GetEntryFor(const std::string& name);
  std::shared_ptr<SpawnedTask> GetSpawnedFor(const std::string& task_id);

  const bool is_enabled_;

  using TasksMap = std::unordered_map<std::string, std::shared_ptr<Entry>>;
  concurrent::Variable<TasksMap> tasks_;

  using SpawnedMap =
      std::unordered_map<std::string, std::shared_ptr<SpawnedTask>>;
  concurrent::Variable<SpawnedMap> spawned_;
};

/// @brief Get reference to TestsuiteTasks instance
/// @param component_context
TestsuiteTasks& GetTestsuiteTasks(
    const components::ComponentContext& component_context);

}  // namespace testsuite

USERVER_NAMESPACE_END
