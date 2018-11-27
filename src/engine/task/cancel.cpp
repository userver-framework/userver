#include <engine/task/cancel.hpp>

#include <cassert>

#include <engine/sleep.hpp>
#include <engine/task/task_context.hpp>

namespace engine {

TaskCancellationBlocker::TaskCancellationBlocker()
    : context_(current_task::GetCurrentTaskContext()),
      was_allowed_(context_->SetCancellable(false)) {}

TaskCancellationBlocker::~TaskCancellationBlocker() {
  assert(context_ == current_task::GetCurrentTaskContext());
  context_->SetCancellable(was_allowed_);
}

namespace current_task {

bool IsCancelRequested() {
  return GetCurrentTaskContext()->IsCancelRequested();
}

void CancellationPoint() {
  auto context = GetCurrentTaskContext();
  if (context->IsCancelRequested() && context->IsCancellable()) {
    engine::Yield();
  }
}

}  // namespace current_task
}  // namespace engine
