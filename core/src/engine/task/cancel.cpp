#include <engine/task/cancel.hpp>

#include <cctype>
#include <exception>

#include <fmt/format.h>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>

#include <engine/sleep.hpp>
#include <logging/log.hpp>
#include <utils/assert.hpp>

#include <engine/task/coro_unwinder.hpp>
#include <engine/task/task_context.hpp>

namespace engine {
namespace {

void Unwind() {
  auto ctx = current_task::GetCurrentTaskContext();
  UASSERT(ctx->GetState() == Task::State::kRunning);

  if (std::uncaught_exceptions()) return;

  if (ctx->SetCancellable(false)) {
    LOG_TRACE() << "Cancelling current task" << logging::LogExtra::Stacktrace();
    // NOLINTNEXTLINE(hicpp-exception-baseclass)
    throw impl::CoroUnwinder{};
  }
}

}  // namespace

namespace current_task {

bool IsCancelRequested() {
  return GetCurrentTaskContext()->IsCancelRequested();
}

bool ShouldCancel() {
  const auto ctx = GetCurrentTaskContext();
  return ctx->IsCancelRequested() && ctx->IsCancellable();
}

TaskCancellationReason CancellationReason() {
  return GetCurrentTaskContext()->CancellationReason();
}

void CancellationPoint() {
  if (current_task::ShouldCancel()) Unwind();
}

}  // namespace current_task

TaskCancellationBlocker::TaskCancellationBlocker()
    : context_(current_task::GetCurrentTaskContext()),
      was_allowed_(context_->SetCancellable(false)) {}

TaskCancellationBlocker::~TaskCancellationBlocker() {
  UASSERT(context_ == current_task::GetCurrentTaskContext());
  context_->SetCancellable(was_allowed_);
}

std::string ToString(TaskCancellationReason reason) {
  static const std::string kNone = "Not cancelled";
  static const std::string kUserRequest = "User request";
  static const std::string kOverload = "Task processor overload";
  static const std::string kAbandoned = "Task destruction before finish";
  static const std::string kShutdown = "Task processor shutdown";
  switch (reason) {
    case TaskCancellationReason::kNone:
      return kNone;
    case TaskCancellationReason::kUserRequest:
      return kUserRequest;
    case TaskCancellationReason::kOverload:
      return kOverload;
    case TaskCancellationReason::kAbandoned:
      return kAbandoned;
    case TaskCancellationReason::kShutdown:
      return kShutdown;
  }
  return fmt::format("unknown({})", static_cast<int>(reason));
}

}  // namespace engine
