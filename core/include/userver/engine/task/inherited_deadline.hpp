#pragma once

/// @file userver/engine/task/inherited_deadline.hpp
/// @brief @copybrief engine::TaskInheritedDeadline

#include <string>

#include <userver/engine/deadline.hpp>

namespace engine {

extern const std::string kTaskInheritedDeadlineKey;

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

/// Remove deadline info from current task storage.
/// Useful in detached tasks.
void ResetCurrentTaskInheritedDeadline();

}  // namespace engine
