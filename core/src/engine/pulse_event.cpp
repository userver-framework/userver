#include <userver/engine/pulse_event.hpp>

#include <utility>

#include <userver/utils/assert.hpp>
#include <userver/utils/impl/internal_tag.hpp>

#include <engine/impl/future_utils.hpp>
#include <engine/impl/wait_list.hpp>
#include <engine/task/task_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

PulseEventSubscription::PulseEventSubscription() noexcept = default;

PulseEventSubscription::PulseEventSubscription(PulseEvent* event, Ticket ticket) noexcept
    : event_(event), ticket_(ticket) {}

PulseEventSubscription::PulseEventSubscription(PulseEventSubscription&& other) noexcept
    : event_(std::exchange(other.event_, nullptr)), ticket_(other.ticket_) {}

PulseEventSubscription& PulseEventSubscription::operator=(PulseEventSubscription&& other) noexcept {
    if (this != &other) {
        event_ = std::exchange(other.event_, nullptr);
        ticket_ = other.ticket_;
    }
    return *this;
}

PulseEventSubscription::~PulseEventSubscription() = default;

bool PulseEventSubscription::IsValid() const noexcept { return event_ != nullptr; }

AwaitableToken PulseEventSubscription::GetAwaitableToken() noexcept USERVER_IMPL_LIFETIME_BOUND {
    if (!IsValid()) {
        return {};
    }
    return AwaitableToken{utils::impl::InternalTag{}, this};
}

FutureStatus PulseEventSubscription::WaitUntil(Deadline deadline) {
    UINVARIANT(IsValid(), "invalid PulseEventSubscription");

    impl::TaskContext& current = current_task::GetCurrentTaskContext();
    return impl::ToFutureStatus(current.Sleep(*this, deadline));
}

bool PulseEventSubscription::IsReady() const noexcept {
    UASSERT(IsValid());
    return event_->generation_.load(std::memory_order_acquire) > ticket_;
}

void PulseEventSubscription::TryAppendAwaiter(impl::AwaiterPtr& awaiter, std::uintptr_t context) {
    UASSERT(IsValid());
    impl::WaitList::Lock lock(*event_->awaiters_);
    if (IsReady()) {
        return;
    }
    event_->awaiters_->Append(lock, std::move(awaiter), context);
}

impl::AwaiterPtr PulseEventSubscription::RemoveAwaiter(impl::Awaiter& awaiter, std::uintptr_t context) noexcept {
    UASSERT(IsValid());
    impl::WaitList::Lock lock(*event_->awaiters_);
    return event_->awaiters_->Remove(lock, awaiter, context);
}

PulseEvent::PulseEvent() noexcept = default;

PulseEvent::~PulseEvent() = default;

PulseEventSubscription PulseEvent::Subscribe() noexcept {
    const auto ticket = generation_.load(std::memory_order_acquire);
    return PulseEventSubscription{this, ticket};
}

void PulseEvent::Send() noexcept {
    impl::WaitList::Lock lock(*awaiters_);
    generation_.fetch_add(1, std::memory_order_release);
    awaiters_->NotifyAll(lock);
}

}  // namespace engine

USERVER_NAMESPACE_END
