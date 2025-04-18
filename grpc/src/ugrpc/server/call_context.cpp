#include <userver/ugrpc/server/call_context.hpp>

#include <userver/ugrpc/server/impl/call.hpp>
#include <userver/utils/impl/internal_tag.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

CallContextBase::CallContextBase(utils::impl::InternalTag, impl::CallAnyBase& call) : call_(call) {}

const impl::CallAnyBase& CallContextBase::GetCall(utils::impl::InternalTag) const { return call_; }

impl::CallAnyBase& CallContextBase::GetCall(utils::impl::InternalTag) { return call_; }

grpc::ServerContext& CallContextBase::GetServerContext() { return call_.GetContext(); }

std::string_view CallContextBase::GetCallName() const { return call_.GetCallName(); }

std::string_view CallContextBase::GetServiceName() const { return call_.GetServiceName(); }

std::string_view CallContextBase::GetMethodName() const { return call_.GetMethodName(); }

tracing::Span& CallContextBase::GetSpan() { return call_.GetSpan(); }

utils::AnyStorage<StorageContext>& CallContextBase::GetStorageContext() { return call_.GetStorageContext(); }

void GenericCallContext::SetMetricsCallName(std::string_view call_name) {
    GetCall(utils::impl::InternalTag{}).SetMetricsCallName(call_name);
}

}  // namespace ugrpc::server

USERVER_NAMESPACE_END
