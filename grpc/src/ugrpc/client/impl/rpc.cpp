#include <userver/ugrpc/client/impl/rpc.hpp>

#include <userver/engine/task/cancel.hpp>

#include <userver/ugrpc/client/exceptions.hpp>
#include <userver/ugrpc/client/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

UnaryFinishFuture::UnaryFinishFuture(UnaryCallState& state, const google::protobuf::Message* response) noexcept
    : state_{state}, response_{response} {}

UnaryFinishFuture::~UnaryFinishFuture() { Destroy(); }

void UnaryFinishFuture::Destroy() noexcept try {
    if (state_.IsFinishProcessed()) {
        return;
    }
    state_.SetFinishProcessed();
    state_.GetClientContext().TryCancel();
    auto& finish = state_.GetFinishAsyncMethodInvocation();

    const engine::TaskCancellationBlocker cancel_blocker;
    const auto wait_status = finish.Wait();

    state_.GetStatsScope().SetFinishTime(finish.GetFinishTime());

    switch (wait_status) {
        case ugrpc::impl::AsyncMethodInvocation::WaitStatus::kOk:
            ProcessFinishAbandoned(state_);
            break;
        case ugrpc::impl::AsyncMethodInvocation::WaitStatus::kError:
            ProcessNetworkError(state_, "Finish");
            break;
        case ugrpc::impl::AsyncMethodInvocation::WaitStatus::kCancelled:
        case ugrpc::impl::AsyncMethodInvocation::WaitStatus::kDeadline:
            utils::AbortWithStacktrace("unreachable");
    }
} catch (const std::exception& ex) {
    LOG_WARNING() << "There is a caught exception in 'UnaryFinishFuture::Destroy': " << ex;
}

bool UnaryFinishFuture::IsReady() const noexcept {
    auto& finish = state_.GetFinishAsyncMethodInvocation();
    return finish.IsReady();
}

engine::FutureStatus UnaryFinishFuture::WaitUntil(engine::Deadline deadline) const noexcept {
    if (state_.IsFinishProcessed()) return engine::FutureStatus::kReady;

    auto& finish = state_.GetFinishAsyncMethodInvocation();
    const auto wait_status = impl::WaitAndTryCancelIfNeeded(finish, deadline, state_.GetClientContext());
    switch (wait_status) {
        case ugrpc::impl::AsyncMethodInvocation::WaitStatus::kOk:
            state_.SetFinishProcessed();
            state_.GetStatsScope().SetFinishTime(finish.GetFinishTime());
            try {
                ProcessFinish(state_, response_);
            } catch (...) {
                exception_ = std::current_exception();
            }
            return engine::FutureStatus::kReady;

        case ugrpc::impl::AsyncMethodInvocation::WaitStatus::kError:
            state_.SetFinishProcessed();
            state_.GetStatsScope().SetFinishTime(finish.GetFinishTime());
            ProcessNetworkError(state_, "Finish");
            exception_ = std::make_exception_ptr(RpcInterruptedError(state_.GetCallName(), "Finish"));
            return engine::FutureStatus::kReady;

        case ugrpc::impl::AsyncMethodInvocation::WaitStatus::kCancelled:
            state_.GetStatsScope().OnCancelled();
            return engine::FutureStatus::kCancelled;

        case ugrpc::impl::AsyncMethodInvocation::WaitStatus::kDeadline:
            return engine::FutureStatus::kTimeout;
    }

    utils::AbortWithStacktrace("should be unreachable");
}

void UnaryFinishFuture::Get() {
    UINVARIANT(!state_.IsStatusExtracted(), "'Get' called multiple times on the same future");
    state_.SetStatusExtracted();

    const auto future_status = WaitUntil(engine::Deadline{});

    if (future_status == engine::FutureStatus::kCancelled) {
        throw RpcCancelledError(state_.GetCallName(), "UnaryFuture::Get");
    }
    UASSERT(state_.IsFinishProcessed());

    if (exception_) {
        std::rethrow_exception(std::exchange(exception_, {}));
    }

    CheckFinishStatus(state_);
}

engine::impl::ContextAccessor* UnaryFinishFuture::TryGetContextAccessor() noexcept {
    // Unfortunately, we can't require that TryGetContextAccessor is not called
    // after future is finished - it doesn't match pattern usage of WaitAny
    // Instead we should return nullptr
    if (state_.IsStatusExtracted()) {
        return nullptr;
    }

    // if state exists, then FinishAsyncMethodInvocation also exists
    auto& finish = state_.GetFinishAsyncMethodInvocation();
    return finish.TryGetContextAccessor();
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
