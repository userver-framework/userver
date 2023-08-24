#include <userver/engine/task/shared_task.hpp>

#include <engine/task/task_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

static_assert(
    !std::is_polymorphic_v<SharedTask>,
    "Slicing is used by derived types, virtual functions would not work.");

SharedTask::SharedTask() = default;

SharedTask::SharedTask(impl::TaskContextHolder&& context)
    : TaskBase(std::move(context)) {}

SharedTask::~SharedTask() { DecrementSharedUsages(); }

SharedTask::SharedTask(const SharedTask& other) noexcept
    : TaskBase(static_cast<const TaskBase&>(other)) {
  IncrementSharedUsages();
}

// NOLINTNEXTLINE(cert-oop54-cpp)
SharedTask& SharedTask::operator=(const SharedTask& other) noexcept {
  if (!HasSameContext(other)) {
    DecrementSharedUsages();
    TaskBase::operator=(other);
    IncrementSharedUsages();
  }
  return *this;
}

SharedTask::SharedTask(SharedTask&& other) noexcept
    : TaskBase(static_cast<TaskBase&&>(other)) {
  UASSERT(!other.IsValid());
}

// NOLINTNEXTLINE(cert-oop54-cpp)
SharedTask& SharedTask::operator=(SharedTask&& other) noexcept {
  if (!HasSameContext(other)) {
    DecrementSharedUsages();
    TaskBase::operator=(std::move(other));
    UASSERT(!other.IsValid());
  }
  return *this;
}

void SharedTask::DecrementSharedUsages() noexcept {
  if (!IsValid()) {
    return;
  }

  const auto usages = GetContext().DecrementFetchSharedTaskUsages();
  UASSERT_MSG(usages != std::numeric_limits<decltype(usages)>::max(),
              "Underflow of reference counter in SharedTask");
  if (usages == 0) {
    Terminate(TaskCancellationReason::kAbandoned);
  }
}

void SharedTask::IncrementSharedUsages() noexcept {
  if (!IsValid()) {
    return;
  }

  [[maybe_unused]] const auto usages =
      GetContext().IncrementFetchSharedTaskUsages();
  UASSERT_MSG(
      usages >= 2,
      "Copied SharedTask from a task that has no alive references to it");
}

}  // namespace engine

USERVER_NAMESPACE_END
