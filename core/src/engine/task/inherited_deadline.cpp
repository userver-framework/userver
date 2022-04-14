#include <userver/engine/task/inherited_deadline.hpp>

#include <userver/engine/task/inherited_variable.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

namespace {

engine::TaskInheritedVariable<std::unique_ptr<TaskInheritedDeadline>>
    kTaskInheritedDeadline;

}  // namespace

TaskInheritedDeadline::TaskInheritedDeadline(Deadline deadline)
    : deadline_(deadline) {}

Deadline TaskInheritedDeadline::GetDeadline() const { return deadline_; }

void TaskInheritedDeadline::SetDeadline(Deadline deadline) {
  deadline_ = deadline;
}

const TaskInheritedDeadline* GetCurrentTaskInheritedDeadlineUnchecked() {
  auto ptr_opt = kTaskInheritedDeadline.GetOptional();
  if (!ptr_opt) return nullptr;
  return ptr_opt->get();
}

void SetCurrentTaskInheritedDeadline(
    std::unique_ptr<TaskInheritedDeadline>&& deadline) {
  kTaskInheritedDeadline.Set(std::move(deadline));
}

void ResetCurrentTaskInheritedDeadline() { kTaskInheritedDeadline.Erase(); }

}  // namespace engine

USERVER_NAMESPACE_END
