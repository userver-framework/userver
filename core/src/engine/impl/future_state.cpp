#include <userver/engine/impl/future_state.hpp>

#include <future>

#include <engine/impl/wait_list_light.hpp>
#include <engine/task/task_context.hpp>
#include <userver/engine/exception.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

namespace {

FutureStatus ToFutureStatus(TaskContext::WakeupSource wakeup_source) {
  switch (wakeup_source) {
    case TaskContext::WakeupSource::kCancelRequest:
      return FutureStatus::kCancelled;
    case TaskContext::WakeupSource::kDeadlineTimer:
      return FutureStatus::kTimeout;
    case TaskContext::WakeupSource::kWaitList:
      return FutureStatus::kReady;
    default:
      UINVARIANT(false, "Unexpected wakeup source in BlockingFuture");
  }
}

}  // namespace

class FutureStateBase::WaitStrategy final : public impl::WaitStrategy {
 public:
  WaitStrategy(FutureStateBase& state, impl::TaskContext& context,
               Deadline deadline)
      : impl::WaitStrategy(deadline), state_(state), context_(context) {}

  void SetupWakeups() override {
    state_.finish_waiters_->Append(&context_);
    if (state_.is_ready_.load()) state_.finish_waiters_->WakeupOne();
  }

  void DisableWakeups() override { state_.finish_waiters_->Remove(context_); }

 private:
  FutureStateBase& state_;
  impl::TaskContext& context_;
};

FutureStateBase::FutureStateBase() noexcept
    : is_ready_(false),
      is_result_store_locked_(false),
      is_future_created_(false) {}

FutureStateBase::~FutureStateBase() = default;

bool FutureStateBase::IsReady() const noexcept { return is_ready_.load(); }

FutureStatus FutureStateBase::WaitUntil(Deadline deadline) {
  if (IsReady()) return FutureStatus::kReady;
  if (deadline.IsReached()) return FutureStatus::kTimeout;

  auto& context = current_task::GetCurrentTaskContext();

  WaitStrategy wait_strategy{*this, context, deadline};
  const auto wakeup_source = context.Sleep(wait_strategy);
  return ToFutureStatus(wakeup_source);
}

void FutureStateBase::OnFutureCreated() {
  if (is_future_created_.exchange(true, std::memory_order_relaxed)) {
    throw std::future_error(std::future_errc::future_already_retrieved);
  }
}

void FutureStateBase::LockResultStore() {
  if (is_result_store_locked_.exchange(true, std::memory_order_relaxed)) {
    throw std::future_error(std::future_errc::promise_already_satisfied);
  }
}

void FutureStateBase::ReleaseResultStore() {
  is_ready_.store(true);
  finish_waiters_->WakeupOne();
}

void FutureStateBase::WaitForResult() {
  const auto wait_result = WaitUntil({});
  if (wait_result == FutureStatus::kCancelled) {
    throw WaitInterruptedException(current_task::CancellationReason());
  }
}

void FutureStateBase::AppendWaiter(impl::TaskContext& context) noexcept {
  finish_waiters_->Append(&context);
}

void FutureStateBase::RemoveWaiter(impl::TaskContext& context) noexcept {
  finish_waiters_->Remove(context);
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
