#include <userver/engine/task/task.hpp>

#include <future>

#include <engine/impl/generic_wait_list.hpp>
#include <engine/task/task_context.hpp>
#include <engine/task/task_processor.hpp>
#include <engine/task/task_processor_pools.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/impl/task_context_holder.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

Task::Task() = default;

Task::Task(impl::TaskContextHolder&& context)
    : context_(std::move(context).Extract()) {
  context_->Wakeup(impl::TaskContext::WakeupSource::kBootstrap,
                   impl::SleepState::Epoch{0});
}

Task::Task(Task&&) noexcept = default;

Task& Task::operator=(Task&& rhs) noexcept {
  Terminate(TaskCancellationReason::kAbandoned);
  context_ = std::move(rhs.context_);
  return *this;
}

Task::Task(const Task&) = default;

Task& Task::operator=(const Task& rhs) {
  if (this == &rhs) return *this;

  Terminate(TaskCancellationReason::kAbandoned);
  context_ = rhs.context_;
  return *this;
}

Task::~Task() { Terminate(TaskCancellationReason::kAbandoned); }

bool Task::IsValid() const { return !!context_; }

Task::State Task::GetState() const {
  return context_ ? context_->GetState() : State::kInvalid;
}

const std::string& Task::GetStateName(State state) {
  static const std::string kInvalid = "kInvalid";
  static const std::string kNew = "kNew";
  static const std::string kQueued = "kQueued";
  static const std::string kRunning = "kRunning";
  static const std::string kSuspended = "kSuspended";
  static const std::string kCancelled = "kCancelled";
  static const std::string kCompleted = "kCompleted";

  switch (state) {
    case State::kInvalid:
      return kInvalid;
    case State::kNew:
      return kNew;
    case State::kQueued:
      return kQueued;
    case State::kRunning:
      return kRunning;
    case State::kSuspended:
      return kSuspended;
    case State::kCancelled:
      return kCancelled;
    case State::kCompleted:
      return kCompleted;
  }

  UINVARIANT(false, "Unexpected Task state");
}

bool Task::IsFinished() const { return context_ && context_->IsFinished(); }

void Task::Wait() const noexcept(false) {
  UASSERT(context_);
  context_->Wait();
}

void Task::WaitUntil(Deadline deadline) const {
  UASSERT(context_);
  context_->WaitUntil(deadline);
}

void Task::Detach() && {
  if (context_) {
    UASSERT(context_->UseCount() > 0);
    // If Adopt throws, the Task is kept in a consistent state
    context_->GetTaskProcessor().Adopt(*context_);
    context_.reset();
  }
}

void Task::RequestCancel() {
  UASSERT(context_);
  context_->RequestCancel(TaskCancellationReason::kUserRequest);
}

void Task::SyncCancel() noexcept {
  Terminate(TaskCancellationReason::kUserRequest);
}

TaskCancellationReason Task::CancellationReason() const {
  UASSERT(context_);
  return context_->CancellationReason();
}

void Task::BlockingWait() const {
  UASSERT(context_);
  UASSERT(current_task::GetCurrentTaskContextUnchecked() == nullptr);

  auto& context = *context_;
  if (context.IsFinished()) return;

  std::packaged_task<void()> task([&context] {
    TaskCancellationBlocker block_cancels;
    context.Wait();
  });
  auto future = task.get_future();

  engine::CriticalAsyncNoSpan(context.GetTaskProcessor(), std::move(task))
      .Detach();
  future.wait();
  future.get();
  UASSERT(context.IsFinished());
}

impl::ContextAccessor* Task::TryGetContextAccessor() noexcept {
  UASSERT(!IsSharedWaitAllowed());
  // Not just context_.get(): upcasting nullptr may produce non-nullptr
  return context_ ? context_.get() : nullptr;
}

void Task::Invalidate() noexcept {
  Terminate(TaskCancellationReason::kAbandoned);
  context_.reset();
}

utils::impl::WrappedCallBase& Task::GetPayload() const noexcept {
  UASSERT(context_);
  return context_->GetPayload();
}

bool Task::IsSharedWaitAllowed() const {
  return IsValid() && context_->IsSharedWaitAllowed();
}

void Task::Terminate(TaskCancellationReason reason) noexcept {
  if (context_ && !IsFinished()) {
    // We are not providing an implicit sync from outside
    // because it's really easy to get a deadlock this way
    // e.g. between global event thread pool and task processor
    context_->RequestCancel(reason);

    TaskCancellationBlocker cancel_blocker;
    Wait();
  }
}

namespace current_task {

TaskProcessor& GetTaskProcessor() {
  return GetCurrentTaskContext().GetTaskProcessor();
}

TaskProcessor* GetTaskProcessorOptional() noexcept {
  auto* const context = GetCurrentTaskContextUnchecked();
  return context ? &context->GetTaskProcessor() : nullptr;
}

ev::ThreadControl& GetEventThread() {
  return GetTaskProcessor().EventThreadPool().NextThread();
}

void AccountSpuriousWakeup() {
  return GetTaskProcessor().GetTaskCounter().AccountSpuriousWakeup();
}

size_t GetStackSize() {
  return GetTaskProcessor()
      .GetTaskProcessorPools()
      ->GetCoroPool()
      .GetStackSize();
}

}  // namespace current_task
}  // namespace engine

USERVER_NAMESPACE_END
