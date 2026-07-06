#include <userver/engine/single_consumer_event.hpp>

#include <engine/impl/future_utils.hpp>
#include <engine/impl/wait_list_light.hpp>
#include <engine/task/task_context.hpp>
#include <userver/engine/awaitable.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

struct SingleConsumerEvent::Impl final : public impl::AwaitableBase {
    enum class IsAutoReset : bool {
        kNo = false,
        kYes = true,
    };

    explicit Impl(IsAutoReset is_auto_reset) noexcept : is_auto_reset(static_cast<bool>(is_auto_reset)) {}

    bool IsReady() const noexcept override { return awaiters.IsSignaled(); }

    void TryAppendAwaiter(impl::AwaiterPtr& awaiter, std::uintptr_t context) override {
        awaiters.GetSignalOrAppend(awaiter, context);
    }

    impl::AwaiterPtr RemoveAwaiter(impl::Awaiter& awaiter, std::uintptr_t context) noexcept override {
        return awaiters.Remove(awaiter, context);
    }

    const bool is_auto_reset;
    impl::WaitListLight awaiters;
};

SingleConsumerEvent::SingleConsumerEvent() noexcept : impl_(Impl::IsAutoReset::kYes) {}

SingleConsumerEvent::SingleConsumerEvent(NoAutoReset) noexcept : impl_(Impl::IsAutoReset::kNo) {}

SingleConsumerEvent::~SingleConsumerEvent() = default;

bool SingleConsumerEvent::IsAutoReset() const noexcept { return impl_->is_auto_reset; }

bool SingleConsumerEvent::WaitForEvent() { return WaitForEventUntil(Deadline{}); }

bool SingleConsumerEvent::WaitForEventUntil(Deadline deadline) { return WaitUntil(deadline) == FutureStatus::kReady; }

FutureStatus SingleConsumerEvent::WaitUntil(Deadline deadline) {
    // optimistic path
    if (IsReady()) {
        if (IsAutoReset()) {
            Reset();
        }
        return FutureStatus::kReady;
    }

    auto& awaiter = current_task::GetCurrentTaskContext();
    const auto wakeup_source = awaiter.Sleep(*impl_, deadline);
    const auto status = impl::ToFutureStatus(wakeup_source);
    if (status != FutureStatus::kReady) {
        return status;
    }

    UASSERT(IsReady());
    if (IsAutoReset()) {
        Reset();
    }
    return FutureStatus::kReady;
}

void SingleConsumerEvent::Reset() noexcept {
    // `GetAndResetSignal` should guarantee at least `std::memory_order_acquire` so that the consumer receives data
    // updates from any additional signals which it resets.
    impl_->awaiters.GetAndResetSignal();
}

void SingleConsumerEvent::Send() { impl_->awaiters.SetSignalAndNotifyOne(); }

bool SingleConsumerEvent::IsReady() const noexcept { return impl_->awaiters.IsSignaled(); }

AwaitableToken SingleConsumerEvent::GetAwaitableToken() {
    // This is a fundamental limitation. Notifications can sometimes get lost
    // (e.g. task woke up due to a deadline or cancellation), so there should be an artifact left behind
    // that signals readiness.
    UINVARIANT(!IsAutoReset(), "SingleConsumerEvent with auto-reset is not supported in WaitAny");

    return AwaitableToken{utils::impl::InternalTag{}, &*impl_};
}

bool SingleConsumerEvent::GetIsSignaled() noexcept {
    if (impl_->is_auto_reset) {
        return impl_->awaiters.GetAndResetSignal();
    } else {
        return impl_->awaiters.IsSignaled();
    }
}

}  // namespace engine

USERVER_NAMESPACE_END
