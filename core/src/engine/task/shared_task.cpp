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
  UASSERT(context_ == other.context_);
  IncrementSharedUsages();
}

// NOLINTNEXTLINE(cert-oop54-cpp)
SharedTask& SharedTask::operator=(const SharedTask& other) noexcept {
  if (context_ != other.context_) {
    DecrementSharedUsages();
    context_ = other.context_;
    IncrementSharedUsages();
  }
  return *this;
}

SharedTask::SharedTask(SharedTask&& other) noexcept
    : TaskBase(static_cast<TaskBase&&>(other)) {
  UASSERT(!other.context_);
  UASSERT(!other.IsValid());
}

// NOLINTNEXTLINE(cert-oop54-cpp)
SharedTask& SharedTask::operator=(SharedTask&& other) noexcept {
  if (context_ != other.context_) {
    DecrementSharedUsages();
    context_ = std::move(other.context_);
    UASSERT(!other.context_);
    UASSERT(!other.IsValid());
  }
  return *this;
}

void SharedTask::DecrementSharedUsages() noexcept {
  if (!context_) {
    return;
  }

  const auto usages = context_->DecrementFetchSharedTaskUsages();
  UASSERT_MSG(usages != std::numeric_limits<decltype(usages)>::max(),
              "Underflow of reference counter in SharedTask");
  if (usages == 0) {
    Terminate(TaskCancellationReason::kAbandoned);
  }
}

void SharedTask::IncrementSharedUsages() noexcept {
  if (!context_) {
    return;
  }

  const auto usages = context_->IncrementFetchSharedTaskUsages();
  UASSERT_MSG(
      usages >= 2,
      "Copied SharedTask from a task that has no alive refereces to it");
}

}  // namespace engine

USERVER_NAMESPACE_END
