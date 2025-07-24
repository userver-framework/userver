#include <userver/ugrpc/impl/async_method_invocation.hpp>

#include <userver/utils/assert.hpp>

#include <userver/engine/task/cancel.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

AsyncMethodInvocation::~AsyncMethodInvocation() { WaitWhileBusy(); }

void AsyncMethodInvocation::Notify(bool ok) noexcept {
    ok_ = ok;
    event_.Send();
}

bool AsyncMethodInvocation::IsBusy() const noexcept { return busy_; }

void* AsyncMethodInvocation::GetCompletionTag() noexcept {
    UASSERT(!busy_);
    busy_ = true;
    return static_cast<EventBase*>(this);
}

AsyncMethodInvocation::WaitStatus AsyncMethodInvocation::Wait() noexcept { return WaitUntil(engine::Deadline{}); }

AsyncMethodInvocation::WaitStatus AsyncMethodInvocation::WaitUntil(engine::Deadline deadline) noexcept {
    if (engine::current_task::ShouldCancel()) {
        // Make sure that cancelled RPC returns kCancelled (significant for tests)
        return WaitStatus::kCancelled;
    }

    const engine::FutureStatus future_status = event_.WaitUntil(deadline);
    switch (future_status) {
        case engine::FutureStatus::kCancelled: {
            return WaitStatus::kCancelled;
        }
        case engine::FutureStatus::kTimeout: {
            return WaitStatus::kDeadline;
        }
        case engine::FutureStatus::kReady: {
            busy_ = false;
            return ok_ ? WaitStatus::kOk : WaitStatus::kError;
        }
    }

    utils::AbortWithStacktrace("should be unreachable");
}

engine::impl::ContextAccessor* AsyncMethodInvocation::TryGetContextAccessor() noexcept {
    return event_.TryGetContextAccessor();
}

bool AsyncMethodInvocation::IsReady() const noexcept { return event_.IsReady(); }

void AsyncMethodInvocation::WaitWhileBusy() noexcept {
    if (busy_) {
        event_.WaitNonCancellable();
    }
    busy_ = false;
}

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
