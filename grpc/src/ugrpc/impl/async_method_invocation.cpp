#include <userver/ugrpc/impl/async_method_invocation.hpp>

#include <userver/utils/assert.hpp>

#include <userver/engine/task/cancel.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

AsyncMethodInvocation::~AsyncMethodInvocation() {
    if (enqueued_) {
        event_.WaitNonCancellable();
    }
}

void AsyncMethodInvocation::Notify(bool ok) noexcept {
    ok_ = ok;
    event_.Send();
}

void* AsyncMethodInvocation::GetCompletionTag() noexcept {
    UASSERT(!enqueued_);
    enqueued_ = true;
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
            return ok_ ? WaitStatus::kOk : WaitStatus::kError;
        }
    }

    utils::AbortWithStacktrace("should be unreachable");
}

bool AsyncMethodInvocation::WaitNonCancellable() noexcept {
    event_.WaitNonCancellable();
    return ok_;
}

engine::impl::ContextAccessor* AsyncMethodInvocation::TryGetContextAccessor() noexcept {
    return event_.TryGetContextAccessor();
}

bool AsyncMethodInvocation::IsReady() const noexcept { return event_.IsReady(); }

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
