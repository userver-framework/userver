#include <userver/ugrpc/client/rpc.hpp>

#include <userver/tracing/tags.hpp>
#include <userver/utils/fast_scope_guard.hpp>

#include <userver/ugrpc/client/exceptions.hpp>
#include <userver/ugrpc/client/middlewares/base.hpp>
#include <userver/ugrpc/status_codes.hpp>

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
    : data_{std::exchange(other.data_, nullptr)}, post_finish_{std::move(other.post_finish_)} {}

UnaryFuture& UnaryFuture::operator=(UnaryFuture&& other) noexcept {
    if (this == &other) return *this;
    [[maybe_unused]] auto for_destruction = std::move(*this);
    data_ = std::exchange(other.data_, nullptr);
    post_finish_ = std::move(other.post_finish_);
    return *this;
}

UnaryFuture::~UnaryFuture() {
    if (data_) {
        impl::RpcData::AsyncMethodInvocationGuard guard(*data_);

        data_->GetContext().TryCancel();

        [[maybe_unused]] const auto future_status = WaitUntil(engine::Deadline{});
    }
}

bool UnaryFuture::IsReady() const noexcept {
    UINVARIANT(data_, "IsReady should be called only before 'Get'");
    auto& finish = data_->GetFinishAsyncMethodInvocation();
    return finish.IsReady();
}

engine::FutureStatus UnaryFuture::WaitUntil(engine::Deadline deadline) const {
    UINVARIANT(data_, "WaitUntil should be called only before 'Get'");
    auto& finish = data_->GetFinishAsyncMethodInvocation();
    const auto wait_status = impl::WaitUntil(finish, data_->GetContext(), deadline);
    switch (wait_status) {
        case impl::AsyncMethodInvocation::WaitStatus::kOk:
            if (!data_->GetAndSetFinishProcessed()) {
                const auto& status = finish.GetStatus();

                data_->GetStatsScope().OnExplicitFinish(status.error_code());
                data_->GetStatsScope().Flush();

                post_finish_(*data_, status);

                const auto& parsed_gstatus = finish.GetParsedGStatus();
                impl::SetStatusDetailsForSpan(data_->GetSpan(), status, parsed_gstatus.gstatus_string);
                data_->ResetSpan();
            }
            return engine::FutureStatus::kReady;

        case impl::AsyncMethodInvocation::WaitStatus::kCancelled:
            data_->GetStatsScope().OnCancelled();
            return engine::FutureStatus::kCancelled;

        case impl::AsyncMethodInvocation::WaitStatus::kDeadline:
            return engine::FutureStatus::kTimeout;

        case impl::AsyncMethodInvocation::WaitStatus::kError:
            UINVARIANT(false, "Client-side Finish: ok should always be true");
    }

    UINVARIANT(false, "unreachable");
}

void UnaryFuture::Get() {
    UINVARIANT(data_, "'Get' should not be called after readiness");
    impl::RpcData::AsyncMethodInvocationGuard guard(*data_);

    const auto future_status = WaitUntil(engine::Deadline{});

    auto* const data = std::exchange(data_, nullptr);

    if (engine::FutureStatus::kCancelled == future_status) {
        throw RpcCancelledError(data->GetCallName(), "UnaryFuture::Get");
    }

    auto& finish = data->GetFinishAsyncMethodInvocation();
    auto& status = finish.GetStatus();
    if (!status.ok()) {
        auto& parsed_gstatus = finish.GetParsedGStatus();
        impl::ThrowErrorWithStatus(
            data->GetCallName(),
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
    if (!data_) {
        return nullptr;
    }

    // if data exists, then FinishAsyncMethodInvocation also exists
    auto& finish = data_->GetFinishAsyncMethodInvocation();
    return finish.TryGetContextAccessor();
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
