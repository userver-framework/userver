#include <userver/ugrpc/client/call.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

grpc::ClientContext& CallAnyBase::GetContext() { return GetData().GetContext(); }

std::string_view CallAnyBase::GetClientName() const { return GetData().GetClientName(); }

std::string_view CallAnyBase::GetCallName() const { return GetData().GetCallName(); }

tracing::Span& CallAnyBase::GetSpan() { return GetData().GetSpan(); }

impl::RpcData& CallAnyBase::GetData() {
    UASSERT(data_);
    return *data_;
}

const impl::RpcData& CallAnyBase::GetData() const {
    UASSERT(data_);
    return *data_;
}

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
