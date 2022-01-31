#include <userver/engine/get_all.hpp>

#include <engine/task/task_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

namespace impl {

GetAllElement::GetAllElement(TaskWithResult<void>& task,
                             Task::ContextAccessor context_accessor)
    : task_{task}, context_accessor_{context_accessor} {}

void GetAllElement::Access() {
  UASSERT(IsTaskFinished());

  if (!std::exchange(was_accessed_, true)) task_.Get();
}

bool GetAllElement::WasAccessed() const { return was_accessed_; }

bool GetAllElement::IsTaskFinished() const {
  return WasAccessed() || task_.IsFinished();
}

Task::ContextAccessor& GetAllElement::GetContextAccessor() {
  return context_accessor_;
}

class GetAllWaitStrategy final : public WaitStrategy {
 public:
  GetAllWaitStrategy(std::vector<GetAllElement>& targets, TaskContext& current)
      : WaitStrategy({}), targets_{targets}, current_{current} {}

  void SetupWakeups() override {
    bool wakeup_called = false;
    for (auto& target : targets_) {
      if (target.WasAccessed()) continue;

      target.GetContextAccessor().AppendWaiter(&current_);
      if (target.IsTaskFinished()) {
        if (!std::exchange(wakeup_called, true)) {
          target.GetContextAccessor().WakeupOneWaiter();
        }
      }
    }
  }

  void DisableWakeups() override {
    for (auto& target : targets_) {
      target.GetContextAccessor().RemoveWaiter(&current_);
    }
  }

 private:
  std::vector<GetAllElement>& targets_;
  TaskContext& current_;
};

void GetAllHelper::GetAll(Container& tasks) {
  auto ga_elements = BuildGetAllElementsFromContainer(tasks);
  DoGetAll(ga_elements);
}

std::vector<GetAllElement> GetAllHelper::BuildGetAllElementsFromContainer(
    Container& tasks) {
  std::vector<GetAllElement> ga_elements;
  ga_elements.reserve(std::size(tasks));

  for (auto& task : tasks) {
    task.EnsureValid();
    ga_elements.emplace_back(task, task.GetContextAccessor());
  }

  return ga_elements;
}

void GetAllHelper::DoGetAll(std::vector<GetAllElement>& ga_elements) {
  auto& current = current_task::GetCurrentTaskContext();
  for (auto& ga_element : ga_elements) {
    if (!ga_element.GetContextAccessor().IsWaitingEnabledFrom(&current)) {
      ReportDeadlock();
    }
  }

  GetAllWaitStrategy wait_manager{ga_elements, current};
  while (true) {
    if (current.ShouldCancel()) {
      throw WaitInterruptedException{current.CancellationReason()};
    }

    bool all_completed = true;
    for (auto& ga_element : ga_elements) {
      const auto is_finished = ga_element.IsTaskFinished();
      if (is_finished) ga_element.Access();

      all_completed &= is_finished;
    }
    if (all_completed) break;

    current.Sleep(wait_manager);
  }
}

}  // namespace impl

void GetAll(std::vector<engine::TaskWithResult<void>>& tasks) {
  impl::GetAllHelper::GetAll(tasks);
};

}  // namespace engine

USERVER_NAMESPACE_END
