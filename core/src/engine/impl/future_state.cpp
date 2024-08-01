#include <userver/engine/impl/future_state.hpp>

#include <future>

#include <engine/impl/future_utils.hpp>
#include <engine/impl/wait_list_light.hpp>
#include <engine/task/task_context.hpp>
#include <userver/engine/exception.hpp>
#include <userver/engine/task/cancel.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

FutureStateBase::FutureStateBase() noexcept
    : is_result_store_locked_(false), is_future_created_(false) {}

FutureStateBase::~FutureStateBase() = default;

bool FutureStateBase::IsReady() const noexcept {
  return finish_waiters_->IsSignaled();
}

FutureStatus FutureStateBase::WaitUntil(Deadline deadline) {
  if (IsReady()) return FutureStatus::kReady;

  auto& context = current_task::GetCurrentTaskContext();

  FutureWaitStrategy wait_strategy{*this, context};
  const auto wakeup_source = context.Sleep(wait_strategy, deadline);
  return ToFutureStatus(wakeup_source);
}

void FutureStateBase::OnFutureCreated() {
  if (is_future_created_.exchange(true, std::memory_order_relaxed)) {
    throw std::future_error(std::future_errc::future_already_retrieved);
  }
}

bool FutureStateBase::IsFutureCreated() const noexcept {
  return is_future_created_.load(std::memory_order_relaxed);
}

void FutureStateBase::LockResultStore() {
  if (is_result_store_locked_.exchange(true, std::memory_order_relaxed)) {
    throw std::future_error(std::future_errc::promise_already_satisfied);
  }
}

void FutureStateBase::ReleaseResultStore() {
  finish_waiters_->SetSignalAndWakeupOne();
}

void FutureStateBase::WaitForResult() {
  const auto wait_result = WaitUntil({});
  if (wait_result == FutureStatus::kCancelled) {
    throw WaitInterruptedException(current_task::CancellationReason());
  }
}

EarlyWakeup FutureStateBase::TryAppendWaiter(TaskContext& waiter) {
  return EarlyWakeup{finish_waiters_->GetSignalOrAppend(&waiter)};
}

void FutureStateBase::RemoveWaiter(TaskContext& context) noexcept {
  finish_waiters_->Remove(context);
}

void FutureStateBase::AfterWait() noexcept {}

}  // namespace engine::impl

USERVER_NAMESPACE_END
