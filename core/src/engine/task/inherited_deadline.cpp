#include <engine/task/inherited_deadline.hpp>

#include <utils/task_inherited_data.hpp>

namespace engine {

const std::string kTaskInheritedDeadlineKey = "task-inherited-deadline";

TaskInheritedDeadline::TaskInheritedDeadline(Deadline deadline)
    : deadline_(deadline) {}

Deadline TaskInheritedDeadline::GetDeadline() const { return deadline_; }

void TaskInheritedDeadline::SetDeadline(Deadline deadline) {
  deadline_ = deadline;
}

const TaskInheritedDeadline* GetCurrentTaskInheritedDeadlineUnchecked() {
  auto ptr_opt = ::utils::GetTaskInheritedDataOptional<
      std::unique_ptr<TaskInheritedDeadline>>(kTaskInheritedDeadlineKey);
  if (!ptr_opt) return nullptr;
  return ptr_opt->get();
}

void ResetCurrentTaskInheritedDeadline() {
  ::utils::EraseTaskInheritedData(kTaskInheritedDeadlineKey);
}

}  // namespace engine
