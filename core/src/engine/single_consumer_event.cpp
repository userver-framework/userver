#include <userver/engine/single_consumer_event.hpp>

#include <engine/impl/wait_list_light.hpp>
#include <engine/task/task_context.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

class SingleConsumerEvent::EventWaitStrategy final : public impl::WaitStrategy {
public:
    EventWaitStrategy(SingleConsumerEvent& event, impl::TaskContext& current)
        : event_(event),
          current_(current)
    {}

    impl::EarlyNotify SetupWakeups() override {
        boost::intrusive_ptr<impl::Awaiter> awaiter_ptr{&current_};
        event_.waiters_->GetSignalOrAppend(awaiter_ptr, current_.GetAwaiterContext());
        return impl::EarlyNotify{awaiter_ptr != nullptr};
    }

    void DisableWakeups() noexcept override { event_.waiters_->Remove(current_, current_.GetAwaiterContext()); }

private:
    SingleConsumerEvent& event_;
    impl::TaskContext& current_;
};

SingleConsumerEvent::SingleConsumerEvent() noexcept = default;

SingleConsumerEvent::SingleConsumerEvent(NoAutoReset) noexcept : is_auto_reset_(false) {}

SingleConsumerEvent::~SingleConsumerEvent() = default;

bool SingleConsumerEvent::IsAutoReset() const noexcept { return is_auto_reset_; }

bool SingleConsumerEvent::WaitForEvent() { return WaitForEventUntil(Deadline{}); }

bool SingleConsumerEvent::WaitForEventUntil(Deadline deadline) {
    if (GetIsSignaled()) {
        return true;  // optimistic path
    }

    impl::TaskContext& current = current_task::GetCurrentTaskContext();
    EventWaitStrategy wait_manager{*this, current};

    while (true) {
        if (GetIsSignaled()) {
            return true;
        }

        const auto wakeup_source = current.Sleep(wait_manager, deadline);
        if (!impl::HasWaitSucceeded(wakeup_source)) {
            return false;
        }
    }
}

void SingleConsumerEvent::Reset() noexcept { waiters_->GetAndResetSignal(); }

void SingleConsumerEvent::Send() { waiters_->SetSignalAndNotifyOne(); }

bool SingleConsumerEvent::IsReady() const noexcept { return waiters_->IsSignaled(); }

bool SingleConsumerEvent::GetIsSignaled() noexcept {
    if (is_auto_reset_) {
        return waiters_->GetAndResetSignal();
    } else {
        return waiters_->IsSignaled();
    }
}

void SingleConsumerEvent::CheckIsAutoResetForWaitPredicate() const {
    UINVARIANT(IsAutoReset(), "Wait with predicate requires auto-reset functionality");
}

}  // namespace engine

USERVER_NAMESPACE_END
