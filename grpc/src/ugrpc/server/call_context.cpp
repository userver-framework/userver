#include <userver/ugrpc/server/call_context.hpp>

#include <userver/ugrpc/server/call.hpp>
#include <userver/utils/impl/internal_tag.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

CallContextBase::CallContextBase(utils::impl::InternalTag, CallAnyBase& call) : call_(call) {}

const CallAnyBase& CallContextBase::GetCall() const { return call_; }

CallAnyBase& CallContextBase::GetCall() { return call_; }

grpc::ServerContext& CallContextBase::GetServerContext() { return GetCall().GetContext(); }

std::string_view CallContextBase::GetCallName() const { return GetCall().GetCallName(); }

std::string_view CallContextBase::GetServiceName() const { return GetCall().GetServiceName(); }

std::string_view CallContextBase::GetMethodName() const { return GetCall().GetMethodName(); }

tracing::Span& CallContextBase::GetSpan() { return GetCall().GetSpan(); }

utils::AnyStorage<StorageContext>& CallContextBase::GetStorageContext() { return GetCall().GetStorageContext(); }

void GenericCallContext::SetMetricsCallName(std::string_view call_name) { GetCall().SetMetricsCallName(call_name); }

}  // namespace ugrpc::server

USERVER_NAMESPACE_END
