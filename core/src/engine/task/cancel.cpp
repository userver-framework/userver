#include <userver/engine/task/cancel.hpp>

#include <exception>

#include <fmt/format.h>
#include <boost/algorithm/string/replace.hpp>

#include <userver/engine/sleep.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>

#include <engine/task/coro_unwinder.hpp>
#include <engine/task/task_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {
namespace {

void Unwind() {
  auto& ctx = current_task::GetCurrentTaskContext();
  UASSERT(ctx.GetState() == Task::State::kRunning);

  if (std::uncaught_exceptions()) return;

  if (ctx.SetCancellable(false)) {
    LOG_TRACE() << "Cancelling current task" << logging::LogExtra::Stacktrace();
    // NOLINTNEXTLINE(hicpp-exception-baseclass)
    throw impl::CoroUnwinder{};
  }
}

}  // namespace

namespace current_task {

bool IsCancelRequested() noexcept {
  // Current task is running, so we do not get scheduled and no exception could
  // happen
  return GetCurrentTaskContext().IsCancelRequested();
}

bool ShouldCancel() noexcept {
  // Current task is running, so we do not get scheduled and no exception
  // could happen
  return GetCurrentTaskContext().ShouldCancel();
}

TaskCancellationReason CancellationReason() noexcept {
  return GetCurrentTaskContext().CancellationReason();
}

void CancellationPoint() {
  if (current_task::ShouldCancel()) Unwind();
}

void SetDeadline(Deadline deadline) {
  GetCurrentTaskContext().SetCancelDeadline(deadline);
}

TaskCancellationToken GetCancellationToken() {
  return TaskCancellationToken(GetCurrentTaskContext());
}

}  // namespace current_task

TaskCancellationBlocker::TaskCancellationBlocker()
    : context_(current_task::GetCurrentTaskContext()),
      was_allowed_(context_.SetCancellable(false)) {}

TaskCancellationBlocker::~TaskCancellationBlocker() {
  UASSERT(context_.IsCurrent());
  context_.SetCancellable(was_allowed_);
}

std::string ToString(TaskCancellationReason reason) {
  static const std::string kNone = "Not cancelled";
  static const std::string kUserRequest = "User request";
  static const std::string kDeadline = "Task deadline reached";
  static const std::string kOverload = "Task processor overload";
  static const std::string kAbandoned = "Task destruction before finish";
  static const std::string kShutdown = "Task processor shutdown";
  switch (reason) {
    case TaskCancellationReason::kNone:
      return kNone;
    case TaskCancellationReason::kUserRequest:
      return kUserRequest;
    case TaskCancellationReason::kDeadline:
      return kDeadline;
    case TaskCancellationReason::kOverload:
      return kOverload;
    case TaskCancellationReason::kAbandoned:
      return kAbandoned;
    case TaskCancellationReason::kShutdown:
      return kShutdown;
  }
  return fmt::format("unknown({})", static_cast<int>(reason));
}

TaskCancellationToken::TaskCancellationToken() noexcept = default;

TaskCancellationToken::TaskCancellationToken(
    impl::TaskContext& context) noexcept
    : context_(&context) {}

TaskCancellationToken::TaskCancellationToken(Task& task)
    : context_(task.context_) {
  UASSERT(context_);
}

// clang-tidy insists on defaulting this,
// gcc complains about exception-specification mismatch with '= default'
// NOLINTNEXTLINE(hicpp-use-equals-default,modernize-use-equals-default)
TaskCancellationToken::TaskCancellationToken(
    const TaskCancellationToken& other) noexcept
    : context_{other.context_} {}

TaskCancellationToken::TaskCancellationToken(TaskCancellationToken&&) noexcept =
    default;

TaskCancellationToken& TaskCancellationToken::operator=(
    const TaskCancellationToken& other) noexcept {
  if (&other != this) {
    context_ = other.context_;
  }

  return *this;
}

TaskCancellationToken& TaskCancellationToken::operator=(
    TaskCancellationToken&&) noexcept = default;

TaskCancellationToken::~TaskCancellationToken() = default;

void TaskCancellationToken::RequestCancel() {
  UASSERT(context_);
  context_->RequestCancel(TaskCancellationReason::kUserRequest);
}

bool TaskCancellationToken::IsValid() const noexcept { return !!context_; }

}  // namespace engine

USERVER_NAMESPACE_END
