#include <userver/engine/impl/condition_variable_any.hpp>

#include <userver/engine/mutex.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/utils/assert.hpp>

#include <engine/impl/wait_list.hpp>
#include <engine/task/task_context.hpp>
#include <engine/task/task_processor.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

void OnConditionVariableSpuriousWakeup() { current_task::GetTaskProcessor().GetTaskCounter().AccountSpuriousWakeup(); }

template <typename MutexType>
class CvWaitStrategy final : public WaitStrategy {
public:
    CvWaitStrategy(WaitList& awaiters, TaskContext& current, std::unique_lock<MutexType>& mutex_lock) noexcept
        : awaiters_(awaiters), awaiter_token_(awaiters_), current_(current), mutex_lock_(mutex_lock) {}

    EarlyNotify SetupWakeups() override {
        UASSERT(mutex_lock_);
        UASSERT(current_.IsCurrent());
        {
            WaitList::Lock awaiters_lock{awaiters_};
            awaiters_.Append(awaiters_lock, &current_, current_.GetAwaiterContext());
        }

        mutex_lock_.unlock();
        // A race is not possible here, because check + Append is performed under
        // mutex_lock_, and user state that defines readiness should only be changed
        // by user under mutex_lock_.
        return EarlyNotify{false};
    }

    void DisableWakeups() noexcept override {
        UASSERT(current_.IsCurrent());

        WaitList::Lock awaiters_lock{awaiters_};
        awaiters_.Remove(awaiters_lock, current_, current_.GetAwaiterContext());
    }

private:
    WaitList& awaiters_;
    const WaitList::AwaitersScopeCounter awaiter_token_;
    TaskContext& current_;
    std::unique_lock<MutexType>& mutex_lock_;
};

template <typename MutexType>
ConditionVariableAny<MutexType>::ConditionVariableAny() = default;

template <typename MutexType>
ConditionVariableAny<MutexType>::~ConditionVariableAny() = default;

template <typename MutexType>
CvStatus ConditionVariableAny<MutexType>::WaitUntil(std::unique_lock<MutexType>& lock, Deadline deadline) {
    UASSERT(lock.owns_lock());

    if (deadline.IsReached()) {
        return CvStatus::kTimeout;
    }

    auto& current = current_task::GetCurrentTaskContext();

    auto wakeup_source = TaskContext::WakeupSource::kNone;
    {
        CvWaitStrategy<MutexType> wait_manager(*awaiters_, current, lock);
        wakeup_source = current.Sleep(wait_manager, deadline);
    }
    // re-lock the mutex after it's been released in SetupWakeups()
    // lock.owns_lock() can occur on an immediate cancellation
    if (!lock) {
        lock.lock();
    }

    switch (wakeup_source) {
        case TaskContext::WakeupSource::kCancelRequest:
            return CvStatus::kCancelled;
        case TaskContext::WakeupSource::kDeadlineTimer:
            return CvStatus::kTimeout;
        case TaskContext::WakeupSource::kNone:
        case TaskContext::WakeupSource::kBootstrap:
            UASSERT(!"invalid wakeup source");
            [[fallthrough]];
        case TaskContext::WakeupSource::kNotify:
            return CvStatus::kNoTimeout;
    }

    UINVARIANT(false, "Unexpected wakeup source in ConditionVariableAny");
}

template <typename MutexType>
void ConditionVariableAny<MutexType>::NotifyOne() {
    if (awaiters_->GetCountOfSleepies()) {
        WaitList::Lock lock(*awaiters_);
        awaiters_->NotifyOne(lock);
    }
}

template <typename MutexType>
void ConditionVariableAny<MutexType>::NotifyAll() {
    if (awaiters_->GetCountOfSleepies()) {
        WaitList::Lock lock(*awaiters_);
        awaiters_->NotifyAll(lock);
    }
}

template class ConditionVariableAny<std::mutex>;
template class ConditionVariableAny<Mutex>;

}  // namespace engine::impl

USERVER_NAMESPACE_END
