#include <userver/ugrpc/server/call_context.hpp>

#include <userver/ugrpc/server/call.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

CallContext::CallContext(CallAnyBase& call) : call_{call} {}

grpc::ServerContext& CallContext::GetServerContext() { return GetCall().GetContext(); }

std::string_view CallContext::GetCallName() const { return GetCall().GetCallName(); }

std::string_view CallContext::GetServiceName() const { return GetCall().GetServiceName(); }

std::string_view CallContext::GetMethodName() const { return GetCall().GetMethodName(); }

utils::AnyStorage<StorageContext>& CallContext::GetStorageContext() { return GetCall().GetStorageContext(); }

const CallAnyBase& CallContext::GetCall() const { return call_; }

CallAnyBase& CallContext::GetCall() { return call_; }

void GenericCallContext::SetMetricsCallName(std::string_view call_name) { GetCall().SetMetricsCallName(call_name); }

}  // namespace ugrpc::server

USERVER_NAMESPACE_END
