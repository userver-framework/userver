#include <userver/ugrpc/server/call_context.hpp>

#include <userver/ugrpc/impl/statistics_storage.hpp>
#include <userver/ugrpc/server/impl/call_state.hpp>
#include <userver/utils/impl/internal_tag.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

CallContextBase::CallContextBase(utils::impl::InternalTag, impl::CallState& state) : state_(state) {}

grpc::ServerContext& CallContextBase::GetServerContext() { return state_.server_context; }

std::string_view CallContextBase::GetCallName() const { return state_.call_name; }

std::string_view CallContextBase::GetServiceName() const { return state_.service_name; }

std::string_view CallContextBase::GetMethodName() const { return state_.method_name; }

tracing::Span& CallContextBase::GetSpan() { return state_.GetSpan(); }

utils::AnyStorage<StorageContext>& CallContextBase::GetStorageContext() { return state_.storage_context; }

void GenericCallContext::SetMetricsCallName(std::string_view call_name) {
    auto& state = GetCallState(utils::impl::InternalTag{});
    state.statistics_scope.RedirectTo(state.statistics_storage.GetGenericStatistics(call_name, std::nullopt));
}

}  // namespace ugrpc::server

USERVER_NAMESPACE_END
