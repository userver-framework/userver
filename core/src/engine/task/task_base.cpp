#include <userver/engine/task/task_base.hpp>

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

static_assert(
    !std::is_destructible_v<TaskBase>,
    "Destructor of TaskBase must remain protected to forbid slicing of derived "
    "types to TaskBase and force implementation in derived classes.");

static_assert(
    !std::is_polymorphic_v<TaskBase>,
    "Slicing is used by derived types, virtual functions would not work.");

TaskBase::TaskBase(impl::TaskContextHolder&& context)
    : context_(std::move(context).Extract()) {
  context_->Wakeup(impl::TaskContext::WakeupSource::kBootstrap,
                   impl::SleepState::Epoch{0});
}

bool TaskBase::IsValid() const { return !!context_; }

Task::State TaskBase::GetState() const {
  return context_ ? context_->GetState() : State::kInvalid;
}

const std::string& TaskBase::GetStateName(State state) {
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

bool TaskBase::IsFinished() const { return context_ && context_->IsFinished(); }

void TaskBase::Wait() const noexcept(false) {
  UASSERT(context_);
  context_->Wait();
}

void TaskBase::WaitUntil(Deadline deadline) const {
  UASSERT(context_);
  context_->WaitUntil(deadline);
}

void TaskBase::RequestCancel() {
  UASSERT(context_);
  context_->RequestCancel(TaskCancellationReason::kUserRequest);
}

void TaskBase::SyncCancel() noexcept {
  Terminate(TaskCancellationReason::kUserRequest);
}

TaskCancellationReason TaskBase::CancellationReason() const {
  UASSERT(context_);
  return context_->CancellationReason();
}

void TaskBase::BlockingWait() const {
  UASSERT(context_);
  UASSERT(!current_task::IsTaskProcessorThread());

  auto& context = *context_;
  if (context.IsFinished()) return;

  std::packaged_task<void()> task([&context] {
    const TaskCancellationBlocker block_cancels;
    context.Wait();
  });
  auto future = task.get_future();

  engine::CriticalAsyncNoSpan(context.GetTaskProcessor(), std::move(task))
      .Detach();
  future.wait();
  future.get();
  UASSERT(context.IsFinished());
}

void TaskBase::Invalidate() noexcept {
  Terminate(TaskCancellationReason::kAbandoned);
  context_.reset();
}

TaskBase::TaskBase() = default;
TaskBase::~TaskBase() = default;

TaskBase::TaskBase(TaskBase&&) noexcept = default;
TaskBase& TaskBase::operator=(TaskBase&&) noexcept = default;

// NOLINTNEXTLINE(hicpp-use-equals-default,modernize-use-equals-default)
TaskBase::TaskBase(const TaskBase& other) noexcept : context_(other.context_) {}

// NOLINTNEXTLINE(hicpp-use-equals-default,modernize-use-equals-default,cert-oop54-cpp)
TaskBase& TaskBase::operator=(const TaskBase& other) noexcept {
  context_ = other.context_;
  return *this;
}

impl::TaskContext& TaskBase::GetContext() const noexcept {
  UASSERT(context_);
  return *context_;
}

bool TaskBase::HasSameContext(const TaskBase& other) const noexcept {
  return context_ == other.context_;
}

utils::impl::WrappedCallBase& TaskBase::GetPayload() const noexcept {
  UASSERT(context_);
  return context_->GetPayload();
}

void TaskBase::Terminate(TaskCancellationReason reason) noexcept {
  if (context_ && !IsFinished()) {
    // We are not providing an implicit sync from outside
    // because it's really easy to get a deadlock this way
    // e.g. between global event thread pool and task processor
    context_->RequestCancel(reason);

    const TaskCancellationBlocker cancel_blocker;
    Wait();
  }
}

namespace current_task {

bool IsTaskProcessorThread() noexcept {
  return GetCurrentTaskContextUnchecked() != nullptr;
}

TaskProcessor& GetTaskProcessor() {
  return GetCurrentTaskContext().GetTaskProcessor();
}

std::size_t GetStackSize() {
  return GetTaskProcessor()
      .GetTaskProcessorPools()
      ->GetCoroPool()
      .GetStackSize();
}

ev::ThreadControl& GetEventThread() {
  return GetTaskProcessor().EventThreadPool().NextThread();
}

}  // namespace current_task

}  // namespace engine

USERVER_NAMESPACE_END
