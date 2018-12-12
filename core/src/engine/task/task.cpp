#include <engine/task/task.hpp>

#include <cassert>

#include <engine/task/cancel.hpp>
#include <logging/log.hpp>

#include <engine/coro/pool.hpp>
#include "task_context.hpp"
#include "task_processor.hpp"

namespace engine {

Task::Task() = default;

Task::Task(impl::TaskContextHolder&& context_holder)
    : context_(context_holder.Release()) {
  context_->Wakeup(impl::TaskContext::WakeupSource::kNone);
}

Task::~Task() { Terminate(); }

Task::Task(Task&&) noexcept = default;

Task& Task::operator=(Task&& rhs) noexcept {
  Terminate();
  context_ = std::move(rhs.context_);
  return *this;
}

bool Task::IsValid() const { return !!context_; }

Task::State Task::GetState() const {
  return context_ ? context_->GetState() : State::kInvalid;
}

bool Task::IsFinished() const { return context_ && context_->IsFinished(); }

void Task::Wait() const noexcept(false) {
  assert(context_);
  context_->Wait();
}

void Task::Detach() && {
  if (context_) {
    assert(context_->use_count() > 0);
    context_->GetTaskProcessor().Adopt(std::move(context_));
  }
}

void Task::RequestCancel() {
  assert(context_);
  context_->RequestCancel(CancellationReason::kUserRequest);
}

Task::CancellationReason Task::GetCancellationReason() const {
  assert(context_);
  return context_->GetCancellationReason();
}

void Task::DoWaitUntil(Deadline deadline) const {
  assert(context_);
  context_->WaitUntil(std::move(deadline));
}

void Task::Terminate() noexcept {
  if (context_) {
    // TODO: it is currently not possible to Wait() from outside of coro
    // use std::promise + Detach() instead
    // do it with caution though as you may get a deadlock
    // e.g. between global event thread pool and task processor
    if (!IsFinished()) {
      context_->RequestCancel(Task::CancellationReason::kAbandoned);
    }

    TaskCancellationBlocker cancel_blocker;
    while (!IsFinished()) Wait();
  }
}

std::string ToString(Task::CancellationReason reason) {
  static const std::string kNone = "Not cancelled";
  static const std::string kUserRequest = "User request";
  static const std::string kOverload = "Task processor overload";
  static const std::string kAbandoned = "Task destruction before finish";
  static const std::string kShutdown = "Task processor shutdown";
  switch (reason) {
    case Task::CancellationReason::kNone:
      return kNone;
    case Task::CancellationReason::kUserRequest:
      return kUserRequest;
    case Task::CancellationReason::kOverload:
      return kOverload;
    case Task::CancellationReason::kAbandoned:
      return kAbandoned;
    case Task::CancellationReason::kShutdown:
      return kShutdown;
  }
  return "unknown(" + std::to_string(static_cast<int>(reason)) + ')';
}

namespace current_task {

TaskProcessor& GetTaskProcessor() {
  return GetCurrentTaskContext()->GetTaskProcessor();
}

ev::ThreadControl& GetEventThread() {
  return GetTaskProcessor().EventThreadPool().NextThread();
}

void AccountSpuriousWakeup() {
  return GetTaskProcessor().GetTaskCounter().AccountSpuriousWakeup();
}

}  // namespace current_task
}  // namespace engine
