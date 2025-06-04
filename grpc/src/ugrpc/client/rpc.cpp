#include <userver/ugrpc/client/response_future.hpp>

#include <userver/engine/task/cancel.hpp>

#include <userver/ugrpc/client/exceptions.hpp>
#include <userver/ugrpc/client/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

UnaryFinishFutureImpl::UnaryFinishFutureImpl(impl::CallState& state, const google::protobuf::Message* response) noexcept
    : state_(&state), response_(response) {
    // We expect that FinishAsyncMethodInvocation was already emplaced
    // For unary future it is done in UnaryCall::FinishAsync
    UASSERT(state_->HoldsFinishAsyncMethodInvocationDebug());
}

UnaryFinishFutureImpl::UnaryFinishFutureImpl(UnaryFinishFutureImpl&& other) noexcept
    // state_ == nullptr signals that *this is empty. Other fields may remain garbage in `other`.
    : state_{std::exchange(other.state_, nullptr)},
      response_(other.response_),
      exception_(std::move(other.exception_)) {}

UnaryFinishFutureImpl& UnaryFinishFutureImpl::operator=(UnaryFinishFutureImpl&& other) noexcept {
    if (this == &other) return *this;
    [[maybe_unused]] auto for_destruction = std::move(*this);
    // state_ == nullptr signals that *this is empty. Other fields may remain garbage in `other`.
    state_ = std::exchange(other.state_, nullptr);
    response_ = other.response_;
    exception_ = std::move(other.exception_);
    return *this;
}

UnaryFinishFutureImpl::~UnaryFinishFutureImpl() {
    if (state_ && !state_->IsFinishProcessed()) {
        state_->GetContext().TryCancel();

        const engine::TaskCancellationBlocker cancel_blocker;
        const auto future_status = WaitUntil(engine::Deadline{});
        UASSERT(future_status == engine::FutureStatus::kReady);

        UASSERT(state_->IsFinishProcessed());
    }
}

bool UnaryFinishFutureImpl::IsReady() const noexcept {
    UASSERT_MSG(state_, "'IsReady' called on a moved out future");
    auto& finish = state_->GetFinishAsyncMethodInvocation();
    return finish.IsReady();
}

engine::FutureStatus UnaryFinishFutureImpl::WaitUntil(engine::Deadline deadline) const noexcept {
    UASSERT_MSG(state_, "'WaitUntil' called on a moved out future");
    if (!state_) return engine::FutureStatus::kReady;

    if (state_->IsFinishProcessed()) return engine::FutureStatus::kReady;

    auto& finish = state_->GetFinishAsyncMethodInvocation();
    const auto wait_status = impl::WaitAndTryCancelIfNeeded(finish, deadline, state_->GetContext());
    switch (wait_status) {
        case impl::AsyncMethodInvocation::WaitStatus::kOk:
        case impl::AsyncMethodInvocation::WaitStatus::kError:
            state_->SetFinishProcessed();
            try {
                UINVARIANT(
                    impl::AsyncMethodInvocation::WaitStatus::kOk == wait_status,
                    "Client-side Finish: ok should always be true"
                );
                state_->GetStatsScope().SetFinishTime(finish.GetFinishTime());
                ProcessFinish(*state_, response_);
            } catch (...) {
                exception_ = std::current_exception();
            }
            return engine::FutureStatus::kReady;

        case impl::AsyncMethodInvocation::WaitStatus::kCancelled:
            state_->GetStatsScope().OnCancelled();
            return engine::FutureStatus::kCancelled;

        case impl::AsyncMethodInvocation::WaitStatus::kDeadline:
            return engine::FutureStatus::kTimeout;
    }

    utils::AbortWithStacktrace("should be unreachable");
}

void UnaryFinishFutureImpl::Get() {
    UINVARIANT(state_, "'Get' called on a moved out future");
    UINVARIANT(!state_->IsStatusExtracted(), "'Get' called multiple times on the same future");
    state_->SetStatusExtracted();

    const auto future_status = WaitUntil(engine::Deadline{});

    if (future_status == engine::FutureStatus::kCancelled) {
        throw RpcCancelledError(state_->GetCallName(), "UnaryFuture::Get");
    }
    UASSERT(state_->IsFinishProcessed());

    if (exception_) {
        std::rethrow_exception(std::exchange(exception_, {}));
    }

    CheckFinishStatus(*state_);
}

engine::impl::ContextAccessor* UnaryFinishFutureImpl::TryGetContextAccessor() noexcept {
    // Unfortunately, we can't require that TryGetContextAccessor is not called
    // after future is finished - it doesn't match pattern usage of WaitAny
    // Instead we should return nullptr
    if (!state_ || state_->IsStatusExtracted()) {
        return nullptr;
    }

    // if state exists, then FinishAsyncMethodInvocation also exists
    auto& finish = state_->GetFinishAsyncMethodInvocation();
    return finish.TryGetContextAccessor();
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
