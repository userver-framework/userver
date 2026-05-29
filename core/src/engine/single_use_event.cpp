#include <userver/engine/single_use_event.hpp>

#include <userver/engine/exception.hpp>
#include <userver/engine/task/cancel.hpp>

#include <engine/impl/future_utils.hpp>
#include <engine/impl/wait_list_light.hpp>
#include <engine/task/task_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

SingleUseEvent::SingleUseEvent() noexcept = default;

SingleUseEvent::~SingleUseEvent() = default;

void SingleUseEvent::Wait() {
    switch (WaitUntil(Deadline{})) {
        case FutureStatus::kReady:
            break;
        case FutureStatus::kTimeout:
            UASSERT_MSG(
                false,
                "Timeout is not expected here due to unreachable "
                "Deadline at Sleep"
            );
            // Never reaches
            break;
        case FutureStatus::kCancelled:
            throw WaitInterruptedException(current_task::CancellationReason());
    }
}

FutureStatus SingleUseEvent::WaitUntil(Deadline deadline) {
    impl::TaskContext& current = current_task::GetCurrentTaskContext();
    impl::FutureWaitStrategy wait_strategy{*this, current};
    const auto wakeup_source = current.Sleep(wait_strategy, deadline);

    // There are no spurious wakeups, because the event is single-use: if a task
    // has ever been notified by this SingleUseEvent, then the task will find
    // the SingleUseEvent ready once it wakes up.
    if (wakeup_source == impl::TaskContext::WakeupSource::kNotify) {
        UASSERT(awaiters_->IsSignaled());
    }
    return impl::ToFutureStatus(wakeup_source);
}

void SingleUseEvent::WaitNonCancellable() noexcept {
    const TaskCancellationBlocker cancellation_blocker;

    switch (WaitUntil(Deadline{})) {
        case FutureStatus::kReady:
            break;
        case FutureStatus::kTimeout:
            UASSERT_MSG(
                false,
                "Timeout is not expected here due to unreachable "
                "Deadline at Sleep"
            );
            break;
        case FutureStatus::kCancelled:
            UASSERT_MSG(
                false,
                "Cancellation should have been blocked "
                "by TaskCancellationBlocker"
            );
            break;
    }
}

void SingleUseEvent::Send() noexcept {
    UASSERT_MSG(!awaiters_->IsSignaled(), "Multiple producers detected for the same SingleUseEvent");
    awaiters_->SetSignalAndNotifyOne();
}

bool SingleUseEvent::IsReady() const noexcept { return awaiters_->IsSignaled(); }

void SingleUseEvent::TryAppendAwaiter(boost::intrusive_ptr<impl::Awaiter>& awaiter, std::uintptr_t context) {
    awaiters_->GetSignalOrAppend(awaiter, context);
}

void SingleUseEvent::RemoveAwaiter(impl::Awaiter& awaiter, std::uintptr_t context) noexcept {
    awaiters_->Remove(awaiter, context);
}

}  // namespace engine

USERVER_NAMESPACE_END
