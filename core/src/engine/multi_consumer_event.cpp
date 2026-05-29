#include <userver/engine/multi_consumer_event.hpp>

#include <userver/engine/exception.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/engine/task/cancel.hpp>

#include <engine/impl/future_utils.hpp>
#include <engine/impl/wait_list.hpp>
#include <engine/task/task_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

MultiConsumerEvent::MultiConsumerEvent() noexcept = default;

MultiConsumerEvent::~MultiConsumerEvent() = default;

void MultiConsumerEvent::Wait() {
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

FutureStatus MultiConsumerEvent::WaitUntil(Deadline deadline) {
    if (IsReady()) {
        return FutureStatus::kReady;
    }
    impl::TaskContext& current = current_task::GetCurrentTaskContext();
    impl::FutureWaitStrategy wait_strategy{*this, current};
    const auto wakeup_source = current.Sleep(wait_strategy, deadline);

    // There are no spurious wakeups, because the event is single-use: if a task
    // has ever been notified by this MultiConsumerEvent, then the task will find
    // the MultiConsumerEvent ready once it wakes up.
    if (wakeup_source == impl::TaskContext::WakeupSource::kNotify) {
        UASSERT(IsReady());
    }
    return impl::ToFutureStatus(wakeup_source);
}

void MultiConsumerEvent::Send() noexcept {
    if (is_ready_.load()) {
        return;
    }
    impl::WaitList::Lock lock(*awaiters_);
    if (is_ready_.load()) {
        return;
    }
    is_ready_.store(true);
    awaiters_->NotifyAll(lock);
}

bool MultiConsumerEvent::IsReady() const noexcept { return is_ready_.load(); }

void MultiConsumerEvent::TryAppendAwaiter(boost::intrusive_ptr<impl::Awaiter>& awaiter, std::uintptr_t context) {
    if (is_ready_.load()) {
        return;
    }
    impl::WaitList::Lock lock(*awaiters_);
    if (is_ready_.load()) {
        return;
    }
    awaiters_->Append(lock, std::move(awaiter), context);
}

void MultiConsumerEvent::RemoveAwaiter(impl::Awaiter& awaiter, std::uintptr_t context) noexcept {
    impl::WaitList::Lock lock(*awaiters_);
    awaiters_->Remove(lock, awaiter, context);
}

}  // namespace engine

USERVER_NAMESPACE_END
