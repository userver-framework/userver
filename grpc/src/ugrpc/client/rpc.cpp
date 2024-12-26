#include <userver/ugrpc/client/rpc.hpp>

#include <future>

#include <userver/engine/task/cancel.hpp>

#include <userver/ugrpc/client/exceptions.hpp>
#include <userver/ugrpc/client/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

namespace impl {

void MiddlewarePipeline::PreStartCall(impl::RpcData& data) {
    MiddlewareCallContext context{data};
    for (const auto& mw : data.GetMiddlewares()) {
        mw->PreStartCall(context);
    }
}

void MiddlewarePipeline::PreSendMessage(impl::RpcData& data, const google::protobuf::Message& message) {
    MiddlewareCallContext context{data};
    for (const auto& mw : data.GetMiddlewares()) {
        mw->PreSendMessage(context, message);
    }
}

void MiddlewarePipeline::PostRecvMessage(impl::RpcData& data, const google::protobuf::Message& message) {
    MiddlewareCallContext context{data};
    for (const auto& mw : data.GetMiddlewares()) {
        mw->PostRecvMessage(context, message);
    }
}

void MiddlewarePipeline::PostFinish(impl::RpcData& data, const grpc::Status& status) {
    MiddlewareCallContext context{data};
    for (const auto& mw : data.GetMiddlewares()) {
        mw->PostFinish(context, status);
    }
}

}  // namespace impl

UnaryFuture::UnaryFuture(
    impl::RpcData& data,
    std::function<void(impl::RpcData& data, const grpc::Status& status)> post_finish
) noexcept
    : data_(&data), post_finish_(std::move(post_finish)) {
    // We expect that FinishAsyncMethodInvocation was already emplaced
    // For unary future it is done in UnaryCall::FinishAsync
    UASSERT(data_->HoldsFinishAsyncMethodInvocationDebug());
}

UnaryFuture::UnaryFuture(UnaryFuture&& other) noexcept
    : data_{std::exchange(other.data_, nullptr)},
      post_finish_{std::move(other.post_finish_)},
      exception_(std::move(other.exception_)) {}

UnaryFuture& UnaryFuture::operator=(UnaryFuture&& other) noexcept {
    if (this == &other) return *this;
    [[maybe_unused]] auto for_destruction = std::move(*this);
    data_ = std::exchange(other.data_, nullptr);
    post_finish_ = std::move(other.post_finish_);
    exception_ = std::move(other.exception_);
    return *this;
}

UnaryFuture::~UnaryFuture() {
    if (data_ && !data_->IsFinishProcessed()) {
        data_->GetContext().TryCancel();

        const engine::TaskCancellationBlocker cancel_blocker;
        const auto future_status = WaitUntil(engine::Deadline{});
        UASSERT(future_status == engine::FutureStatus::kReady);

        UASSERT(data_->IsFinishProcessed());
    }
}

bool UnaryFuture::IsReady() const noexcept {
    UASSERT_MSG(data_, "'IsReady' called on a moved out future");
    auto& finish = data_->GetFinishAsyncMethodInvocation();
    return finish.IsReady();
}

engine::FutureStatus UnaryFuture::WaitUntil(engine::Deadline deadline) const noexcept {
    UASSERT_MSG(data_, "'WaitUntil' called on a moved out future");
    if (!data_) return engine::FutureStatus::kReady;

    if (data_->IsFinishProcessed()) return engine::FutureStatus::kReady;

    auto& finish = data_->GetFinishAsyncMethodInvocation();
    const auto wait_status = impl::WaitUntil(finish, data_->GetContext(), deadline);

    switch (wait_status) {
        case impl::AsyncMethodInvocation::WaitStatus::kOk: {
            data_->SetFinishProcessed();
            try {
                ProcessFinish();
            } catch (...) {
                exception_ = std::current_exception();
            }
            return engine::FutureStatus::kReady;
        }

        case impl::AsyncMethodInvocation::WaitStatus::kError:
            utils::impl::AbortWithStacktrace("Client-side Finish: ok should always be true");

        case impl::AsyncMethodInvocation::WaitStatus::kCancelled:
            data_->GetStatsScope().OnCancelled();
            return engine::FutureStatus::kCancelled;

        case impl::AsyncMethodInvocation::WaitStatus::kDeadline:
            return engine::FutureStatus::kTimeout;
    }

    utils::impl::AbortWithStacktrace("Invalid WaitStatus");
}

void UnaryFuture::Get() {
    UINVARIANT(data_, "'Get' called on a moved out future");
    UINVARIANT(!data_->IsStatusExtracted(), "'Get' called multiple times on the same future");
    data_->SetStatusExtracted();

    const auto future_status = WaitUntil(engine::Deadline{});

    if (future_status == engine::FutureStatus::kCancelled) {
        throw RpcCancelledError(data_->GetCallName(), "UnaryFuture::Get");
    }
    UASSERT(data_->IsFinishProcessed());

    if (exception_) {
        std::rethrow_exception(std::exchange(exception_, {}));
    }

    auto& status = data_->GetStatus();
    if (!status.ok()) {
        auto& parsed_gstatus = data_->GetParsedGStatus();
        impl::ThrowErrorWithStatus(
            data_->GetCallName(),
            std::move(status),
            std::move(parsed_gstatus.gstatus),
            std::move(parsed_gstatus.gstatus_string)
        );
    }
}

engine::impl::ContextAccessor* UnaryFuture::TryGetContextAccessor() noexcept {
    // Unfortunately, we can't require that TryGetContextAccessor is not called
    // after future is finished - it doesn't match pattern usage of WaitAny
    // Instead we should return nullptr
    if (!data_ || data_->IsStatusExtracted()) {
        return nullptr;
    }

    // if data exists, then FinishAsyncMethodInvocation also exists
    auto& finish = data_->GetFinishAsyncMethodInvocation();
    return finish.TryGetContextAccessor();
}

void UnaryFuture::ProcessFinish() const {
    UASSERT(data_);

    const auto& status = data_->GetStatus();

    data_->GetStatsScope().OnExplicitFinish(status.error_code());
    data_->GetStatsScope().Flush();

    const auto& parsed_gstatus = data_->GetParsedGStatus();
    impl::SetStatusDetailsForSpan(data_->GetSpan(), status, parsed_gstatus.gstatus_string);

    // TODO pack middleware exceptions into status
    post_finish_(*data_, status);

    data_->ResetSpan();
}

grpc::ClientContext& CallAnyBase::GetContext() { return data_->GetContext(); }

impl::RpcData& CallAnyBase::GetData() {
    UASSERT(data_);
    return *data_;
}

std::string_view CallAnyBase::GetCallName() const {
    UASSERT(data_);
    return data_->GetCallName();
}

std::string_view CallAnyBase::GetClientName() const {
    UASSERT(data_);
    return data_->GetClientName();
}

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
