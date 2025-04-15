#include <userver/ugrpc/server/impl/call.hpp>

#include <boost/range/adaptor/reversed.hpp>

#include <userver/tracing/tags.hpp>
#include <userver/utils/algo.hpp>

#include <userver/ugrpc/impl/statistics_storage.hpp>
#include <userver/ugrpc/server/impl/exceptions.hpp>
#include <userver/ugrpc/server/middlewares/base.hpp>
#include <userver/ugrpc/status_codes.hpp>

#include <ugrpc/impl/internal_tag.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

std::unique_lock<engine::SingleWaitingTaskMutex> CallAnyBase::TakeMutexIfBidirectional() {
    if (call_kind_ == impl::CallKind::kBidirectionalStream) {
        // In stream -> stream RPCs, Recv and Send hooks will naturally run in parallel.
        // However, this can cause data races when:
        // * accessing StorageContext;
        // * accessing Span (AddTag);
        // * accessing ServerContext (e.g. setting metadata);
        // * calling SetError.
        //
        // For now, this mutex lock mitigates most of these issues.
        // There is still some data race potential remaining:
        // * if a PostRecvMessage hook writes to StorageContext and user Send task reads from the same key in parallel,
        //   or vice versa, or same with PreSendMessage hook;
        // * if user code sets metadata in ServerContext in parallel with a PostRecvMessage or PreSendMessage middleware
        //   hook.
        return std::unique_lock(mutex_);
    }
    return {};
}

std::string_view CallAnyBase::GetServiceName() const { return params_.service_name; }

std::string_view CallAnyBase::GetMethodName() const { return params_.method_name; }

void CallAnyBase::SetMetricsCallName(std::string_view call_name) {
    UASSERT_MSG(!call_name.empty(), "call_name must NOT be empty");
    UASSERT_MSG(call_name[0] != '/', utils::StrCat("call_name must NOT start with /, given: ", call_name));
    UASSERT_MSG(
        call_name.find('/') != std::string_view::npos, utils::StrCat("call_name must contain /, given: ", call_name)
    );

    params_.statistics.RedirectTo(params_.statistics_storage.GetGenericStatistics(call_name, std::nullopt));
}

ugrpc::impl::RpcStatisticsScope& CallAnyBase::GetStatistics(ugrpc::impl::InternalTag) { return params_.statistics; }

void CallAnyBase::ApplyRequestHook(google::protobuf::Message* request) {
    UINVARIANT(middleware_call_context_, "CallContext must be invoked");
    if (request) {
        auto lock = TakeMutexIfBidirectional();

        for (const auto& middleware : params_.middlewares) {
            middleware->PostRecvMessage(*middleware_call_context_, *request);
            auto& status = middleware_call_context_->GetStatus();
            if (!status.ok()) {
                throw impl::MiddlewareRpcInterruptionError(std::exchange(status, grpc::Status{}));
            }
        }
    }
}

void CallAnyBase::ApplyResponseHook(google::protobuf::Message* response) {
    UINVARIANT(middleware_call_context_, "CallContext must be invoked");
    if (response) {
        auto lock = TakeMutexIfBidirectional();

        for (const auto& middleware : boost::adaptors::reverse(params_.middlewares)) {
            middleware->PreSendMessage(*middleware_call_context_, *response);
            auto& status = middleware_call_context_->GetStatus();
            if (!status.ok()) {
                throw impl::MiddlewareRpcInterruptionError(std::exchange(status, grpc::Status{}));
            }
        }
    }
}

void CallAnyBase::PostFinish(const grpc::Status& status) noexcept {
    try {
        params_.statistics.OnExplicitFinish(status.error_code());

        auto& span = params_.call_span;
        span.AddTag("grpc_code", ugrpc::ToString(status.error_code()));
        if (!status.ok()) {
            span.AddTag(tracing::kErrorFlag, true);
            span.AddTag(tracing::kErrorMessage, status.error_message());
            span.SetLogLevel(IsServerError(status.error_code()) ? logging::Level::kError : logging::Level::kWarning);
        }
    } catch (const std::exception& ex) {
        LOG_ERROR() << "Error in CallAnyBase::PostFinish: " << ex;
    }
}

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
