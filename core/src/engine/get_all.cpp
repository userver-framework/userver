#include <userver/engine/get_all.hpp>

#include <engine/task/task_context.hpp>
#include <userver/utils/enumerate.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

namespace impl {

void VoidCallback(Task& task, std::size_t /*task_idx*/,
                  void* /*callback_data*/) {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
  auto* void_task = static_cast<TaskWithResult<void>*>(&task);
  void_task->Get();
}

GetAllElement::GetAllElement(Task& task) : task_(task) {}

void GetAllElement::Access(Callback callback, std::size_t task_idx,
                           void* callback_data) {
  UASSERT(IsTaskFinished());
  UASSERT(callback != nullptr);

  if (!std::exchange(was_accessed_, true)) {
    callback(task_, task_idx, callback_data);
  };
}

bool GetAllElement::WasAccessed() const { return was_accessed_; }

bool GetAllElement::IsTaskFinished() const {
  return WasAccessed() || task_.IsFinished();
}

ContextAccessor* GetAllElement::TryGetContextAccessor() {
  return task_.TryGetContextAccessor();
}

class GetAllWaitStrategy final : public WaitStrategy {
 public:
  GetAllWaitStrategy(std::vector<GetAllElement>& targets, TaskContext& current)
      : WaitStrategy({}), targets_{targets}, current_{current} {}

  void SetupWakeups() override {
    bool wakeup_called = false;
    for (auto& target : targets_) {
      if (target.WasAccessed()) continue;

      auto context_acessor = target.TryGetContextAccessor();
      UASSERT(context_acessor);
      context_acessor->AppendWaiter(current_);
      if (target.IsTaskFinished()) {
        if (!std::exchange(wakeup_called, true)) {
          context_acessor->WakeupAllWaiters();
        }
      }
    }
  }

  void DisableWakeups() override {
    for (auto& target : targets_) {
      if (!target.WasAccessed()) {
        auto context_acessor = target.TryGetContextAccessor();
        UASSERT(context_acessor);
        context_acessor->RemoveWaiter(current_);
      }
    }
  }

 private:
  std::vector<GetAllElement>& targets_;
  TaskContext& current_;
};

void GetAllHelper::GetAll(Container<void>& tasks) {
  auto ga_elements = BuildGetAllElementsFromContainer(tasks);
  DoGetAll(ga_elements, VoidCallback, nullptr);
}

void GetAllHelper::DoGetAll(std::vector<GetAllElement>& ga_elements,
                            Callback cb, void* callback_data) {
  auto& current = current_task::GetCurrentTaskContext();
  for (auto& ga_element : ga_elements) {
    auto context_acessor = ga_element.TryGetContextAccessor();
    UASSERT(context_acessor);
  }

  GetAllWaitStrategy wait_manager{ga_elements, current};
  while (true) {
    if (current.ShouldCancel()) {
      throw WaitInterruptedException{current.CancellationReason()};
    }

    bool all_completed = true;
    for (auto [idx, ga_element] : utils::enumerate(ga_elements)) {
      const auto is_finished = ga_element.IsTaskFinished();
      if (is_finished) ga_element.Access(cb, idx, callback_data);

      all_completed &= is_finished;
    }
    if (all_completed) break;

    current.Sleep(wait_manager);
  }
}

}  // namespace impl

void GetAll(std::vector<engine::TaskWithResult<void>>& tasks) {
  impl::GetAllHelper::GetAll(tasks);
}

}  // namespace engine

USERVER_NAMESPACE_END
