#pragma once

/// @file userver/engine/task/inherited_deadline.hpp
/// @brief @copybrief engine::TaskInheritedDeadline

#include <memory>
#include <string>

#include <userver/engine/deadline.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

/// @brief Base class for deadline data to store it in TaskInheritedStorage.
class TaskInheritedDeadline {
 public:
  TaskInheritedDeadline() = default;
  explicit TaskInheritedDeadline(Deadline deadline);
  virtual ~TaskInheritedDeadline() = default;

  void SetDeadline(Deadline deadline);

  Deadline GetDeadline() const;

 private:
  Deadline deadline_{};
};

/// @return pointer to deadline data or nullptr if no deadline is stored for
/// current task.
const TaskInheritedDeadline* GetCurrentTaskInheritedDeadlineUnchecked();

/// Set the deadline data, for use in userver handler implementations
void SetCurrentTaskInheritedDeadline(
    std::unique_ptr<TaskInheritedDeadline>&& deadline);

/// Remove deadline info from current task storage.
/// Useful in detached tasks.
void ResetCurrentTaskInheritedDeadline();

}  // namespace engine

USERVER_NAMESPACE_END
