#include <userver/ugrpc/server/call.hpp>

#include <boost/range/adaptor/reversed.hpp>

#include <userver/logging/impl/logger_base.hpp>
#include <userver/logging/logger.hpp>
#include <userver/ugrpc/impl/statistics_storage.hpp>
#include <userver/ugrpc/server/impl/exceptions.hpp>
#include <userver/ugrpc/server/middlewares/base.hpp>
#include <userver/utils/algo.hpp>

#include <ugrpc/impl/internal_tag.hpp>
#include <ugrpc/server/impl/format_log_message.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

void CallAnyBase::SetMetricsCallName(std::string_view call_name) {
    UASSERT_MSG(!call_name.empty(), "call_name must NOT be empty");
    UASSERT_MSG(call_name[0] != '/', utils::StrCat("call_name must NOT start with /, given: ", call_name));
    UASSERT_MSG(
        call_name.find('/') != std::string_view::npos, utils::StrCat("call_name must contain /, given: ", call_name)
    );

    params_.statistics.RedirectTo(params_.statistics_storage.GetGenericStatistics(call_name, std::nullopt));
}

ugrpc::impl::RpcStatisticsScope& CallAnyBase::GetStatistics(ugrpc::impl::InternalTag) { return params_.statistics; }

void CallAnyBase::LogFinish(grpc::Status status) const {
    constexpr auto kLevel = logging::Level::kInfo;
    if (!params_.access_tskv_logger.ShouldLog(kLevel)) {
        return;
    }

    auto str = impl::FormatLogMessage(
        params_.context.client_metadata(),
        params_.context.peer(),
        params_.call_span.GetStartSystemTime(),
        params_.call_name,
        status.error_code()
    );
    params_.access_tskv_logger.Log(logging::Level::kInfo, std::move(str));
}

void CallAnyBase::ApplyRequestHook(google::protobuf::Message* request) {
    UINVARIANT(middleware_call_context_, "MiddlewareCallContext must be invoked");
    if (request) {
        for (const auto& middleware : params_.middlewares) {
            middleware->CallRequestHook(*middleware_call_context_, *request);
            if (IsFinished()) throw impl::MiddlewareRpcInterruptionError();
        }
    }
}

void CallAnyBase::ApplyResponseHook(google::protobuf::Message* response) {
    UINVARIANT(middleware_call_context_, "MiddlewareCallContext must be invoked");
    if (response) {
        for (const auto& middleware : boost::adaptors::reverse(params_.middlewares)) {
            middleware->CallResponseHook(*middleware_call_context_, *response);
            if (IsFinished()) throw impl::MiddlewareRpcInterruptionError();
        }
    }
}

void CallAnyBase::RunMiddlewarePipeline(utils::impl::InternalTag, MiddlewareCallContext& md_call_context) {
    middleware_call_context_ = &md_call_context;
    md_call_context.Next();
}

std::string_view CallAnyBase::GetServiceName() const { return params_.service_name; }

std::string_view CallAnyBase::GetMethodName() const { return params_.method_name; }

}  // namespace ugrpc::server

USERVER_NAMESPACE_END
