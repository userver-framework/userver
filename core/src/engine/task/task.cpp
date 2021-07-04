#include <userver/engine/task/task.hpp>

#include <future>

#include <engine/coro/pool.hpp>
#include <engine/task/task_context.hpp>
#include <engine/task/task_processor.hpp>
#include <userver/utils/assert.hpp>

#include <userver/engine/async.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/logging/log.hpp>

namespace engine {

Task::Task() = default;

Task::Task(impl::TaskContextHolder&& context_holder)
    : context_(context_holder.Release()) {
  context_->Wakeup(impl::TaskContext::WakeupSource::kBootstrap,
                   impl::SleepState::Epoch{0});
}

Task::~Task() { Terminate(TaskCancellationReason::kAbandoned); }

Task::Task(Task&&) noexcept = default;

Task& Task::operator=(Task&& rhs) noexcept {
  Terminate(TaskCancellationReason::kAbandoned);
  context_ = std::move(rhs.context_);
  return *this;
}

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
    UASSERT(context_->use_count() > 0);
    context_->GetTaskProcessor().Adopt(std::move(context_));
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
  UASSERT(current_task::GetCurrentTaskContextUnchecked() == nullptr);

  if (context_->IsFinished()) return;

  auto context = context_.get();
  std::packaged_task<void()> task([context] { context->Wait(); });
  auto future = task.get_future();

  engine::impl::CriticalAsync(context->GetTaskProcessor(), std::move(task))
      .Detach();
  future.wait();

  while (!context_->IsFinished()) {
    // context->Wait() in task was interrupted. It is possible only in case of
    // TaskProcessor shutdown. context_ will stop soon, but we have no reliable
    // tool to notify this thread when it is finished. So poll context until it
    // is finished.
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
}

void Task::Invalidate() {
  Terminate(TaskCancellationReason::kAbandoned);
  context_.reset();
}

void Task::Terminate(TaskCancellationReason reason) noexcept {
  if (context_ && !IsFinished()) {
    // We are not providing an implicit sync from outside
    // because it's really easy to get a deadlock this way
    // e.g. between global event thread pool and task processor
    context_->RequestCancel(reason);

    TaskCancellationBlocker cancel_blocker;
    while (!IsFinished()) Wait();
  }
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

size_t GetStackSize() { return coro::kStackSize; }

}  // namespace current_task
}  // namespace engine
