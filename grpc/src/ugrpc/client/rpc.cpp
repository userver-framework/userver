#include <userver/ugrpc/client/rpc.hpp>

#include <userver/utils/fast_scope_guard.hpp>

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
    : impl_(data), post_finish_(std::move(post_finish)) {
    // We expect that FinishAsyncMethodInvocation was already emplaced
    // For unary future it is done in UnaryCall::FinishAsync
    UASSERT(data.HoldsFinishAsyncMethodInvocationDebug());
}

UnaryFuture::~UnaryFuture() noexcept {
    if (auto* const data = impl_.GetData()) {
        impl::RpcData::AsyncMethodInvocationGuard guard(*data);

        auto& finish = data->GetFinishAsyncMethodInvocation();

        data->GetContext().TryCancel();

        const auto wait_status = impl::Wait(finish, data->GetContext());

        switch (wait_status) {
            case impl::AsyncMethodInvocation::WaitStatus::kOk:
                [[fallthrough]];
            case impl::AsyncMethodInvocation::WaitStatus::kError:
                impl::ProcessFinishResult(
                    *data,
                    wait_status,
                    std::move(finish.GetStatus()),
                    std::move(finish.GetParsedGStatus()),
                    post_finish_,
                    false
                );
                break;
            case impl::AsyncMethodInvocation::WaitStatus::kCancelled:
                data->GetStatsScope().OnCancelled();
                break;
            case impl::AsyncMethodInvocation::WaitStatus::kDeadline:
                UASSERT_MSG(false, "Unexpected status 'kDeadline' at UnaryFuture destruction");
                break;
        }
    }
}

UnaryFuture& UnaryFuture::operator=(UnaryFuture&& other) noexcept {
    if (this == &other) return *this;
    [[maybe_unused]] auto for_destruction = std::move(*this);
    impl_ = std::move(other.impl_);
    post_finish_ = std::move(other.post_finish_);
    return *this;
}

void UnaryFuture::Get() {
    auto* const data = impl_.GetData();
    UINVARIANT(data, "'Get' should not be called after readiness");

    const auto result = Get(engine::Deadline{});
    UASSERT_MSG(result != engine::FutureStatus::kTimeout, "kTimeout has happened for infinite timeout");

    if (result == engine::FutureStatus::kCancelled) {
        UASSERT_MSG(!impl_.GetData(), "Data should be cleaned up before RpcCancelledError generation");
        throw RpcCancelledError(data->GetCallName(), "Get()");
    }
}

engine::FutureStatus UnaryFuture::Get(engine::Deadline deadline) {
    auto* const data = impl_.GetData();
    UINVARIANT(data, "'Get' should not be called after readiness");
    impl::RpcData::AsyncMethodInvocationGuard guard(*data);

    auto& finish = data->GetFinishAsyncMethodInvocation();

    const auto wait_status = impl::WaitUntil(finish, data->GetContext(), deadline);

    // if result is not ready yet we should not clear data
    if (wait_status == impl::AsyncMethodInvocation::WaitStatus::kDeadline) {
        // If the result is not ready we do not change AsyncMethodInvocation state.
        //
        // It is vital to hold the state of the AsyncMethodInvocation in order to
        // allow subsequent execution of the 'Get' to finalize RPC and exclude
        // operations with incorrect AsyncMethodInvocation from gRPC thread.
        //
        // If for some reason the RPC is not finished via 'Get', destructor should
        // do it and correct AsyncMethodInvocation object is required for actions in
        // destructor.
        guard.Disarm();
    } else {
        // In this case operation has been finished and we got the notification from
        // AsyncMethodInvocation.
        // All used data could be cleared as it is not required anymore.
        // AsyncMethodInvocation object also should be cleared and in result
        // destructor will not wait any finalization from it.
        impl_.ClearData();
    }

    switch (wait_status) {
        case impl::AsyncMethodInvocation::WaitStatus::kOk:
            [[fallthrough]];
        case impl::AsyncMethodInvocation::WaitStatus::kError:
            impl::ProcessFinishResult(
                *data,
                wait_status,
                std::move(finish.GetStatus()),
                std::move(finish.GetParsedGStatus()),
                post_finish_,
                true
            );
            return engine::FutureStatus::kReady;
        case impl::AsyncMethodInvocation::WaitStatus::kCancelled:
            data->GetStatsScope().OnCancelled();
            return engine::FutureStatus::kCancelled;
        case impl::AsyncMethodInvocation::WaitStatus::kDeadline:
            return engine::FutureStatus::kTimeout;
    }

    UASSERT(false);
    return engine::FutureStatus::kTimeout;
}

engine::impl::ContextAccessor* UnaryFuture::TryGetContextAccessor() noexcept {
    // Unfortunately, we can't require that TryGetContextAccessor is not called
    // after future is finished - it doesn't match pattern usage of WaitAny
    // Instead we should return nullptr
    auto* const data = impl_.GetData();
    if (!data) {
        return nullptr;
    }

    // if data exists, then FinishAsyncMethodInvocation also exists
    auto& finish = data->GetFinishAsyncMethodInvocation();
    return finish.TryGetContextAccessor();
}

bool UnaryFuture::IsReady() const noexcept { return impl_.IsReady(); }

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
