#include <userver/ugrpc/server/impl/async_method_invocation.hpp>

#include <userver/engine/task/cancel.hpp>
#include <userver/utils/assert.hpp>

#include <userver/ugrpc/server/exceptions.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

RpcFinishedEvent::RpcFinishedEvent(
    engine::TaskCancellationToken cancellation_token,
    grpc::ServerContext& server_ctx
) noexcept
    : cancellation_token_(std::move(cancellation_token)), server_ctx_(server_ctx) {}

void* RpcFinishedEvent::GetCompletionTag() noexcept { return this; }

void RpcFinishedEvent::Wait() noexcept { event_.WaitNonCancellable(); }

void RpcFinishedEvent::Notify(bool ok) noexcept {
    // From the documentation to grpcpp: Server-side AsyncNotifyWhenDone:
    // ok should always be true
    UASSERT(ok);
    if (server_ctx_.IsCancelled()) {
        cancellation_token_.RequestCancel();
    }
    event_.Send();
}

ugrpc::impl::AsyncMethodInvocation::WaitStatus Wait(ugrpc::impl::AsyncMethodInvocation& async) noexcept {
    using WaitStatus = ugrpc::impl::AsyncMethodInvocation::WaitStatus;

    const engine::TaskCancellationBlocker blocker;
    const auto status = async.Wait();

    switch (status) {
        case WaitStatus::kOk:
        case WaitStatus::kError:
            return status;

        case WaitStatus::kCancelled:
        case WaitStatus::kDeadline:
            UASSERT(false);
    }

    utils::AbortWithStacktrace("should be unreachable");
}

[[nodiscard]] bool IsInvocationSuccessful(ugrpc::impl::AsyncMethodInvocation::WaitStatus status) noexcept {
    switch (status) {
        case ugrpc::impl::AsyncMethodInvocation::WaitStatus::kOk:
            return true;

        case ugrpc::impl::AsyncMethodInvocation::WaitStatus::kError:
            return false;

        case ugrpc::impl::AsyncMethodInvocation::WaitStatus::kDeadline:
            utils::AbortWithStacktrace("Deadline happened on server operation");

        case ugrpc::impl::AsyncMethodInvocation::WaitStatus::kCancelled:
            utils::AbortWithStacktrace("Cancel happened on server operation");
    }

    utils::AbortWithStacktrace("should be unreachable");
}

void CheckInvocationSuccessful(
    ugrpc::impl::AsyncMethodInvocation::WaitStatus status,
    std::string_view call_name,
    std::string_view stage
) {
    switch (status) {
        case ugrpc::impl::AsyncMethodInvocation::WaitStatus::kOk:
            return;

        case ugrpc::impl::AsyncMethodInvocation::WaitStatus::kError:
            throw RpcInterruptedError(call_name, stage);

        case ugrpc::impl::AsyncMethodInvocation::WaitStatus::kDeadline:
            utils::AbortWithStacktrace("Deadline happened on server operation");

        case ugrpc::impl::AsyncMethodInvocation::WaitStatus::kCancelled:
            utils::AbortWithStacktrace("Cancel happened on server operation");
    }

    utils::AbortWithStacktrace("should be unreachable");
}

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
