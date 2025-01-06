#include <userver/ugrpc/client/middlewares/base.hpp>

#include <userver/ugrpc/client/impl/async_methods.hpp>

#include <ugrpc/impl/internal_tag.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

void MiddlewareBase::PreStartCall(MiddlewareCallContext& /*context*/) const {}

void MiddlewareBase::PostFinish(MiddlewareCallContext& /*context*/, const grpc::Status& /*status*/) const {}

void MiddlewareBase::PreSendMessage(MiddlewareCallContext& /*context*/, const google::protobuf::Message& /*message*/)
    const {}

void MiddlewareBase::PostRecvMessage(MiddlewareCallContext& /*context*/, const google::protobuf::Message& /*message*/)
    const {}

MiddlewareBase::MiddlewareBase() = default;

MiddlewareBase::~MiddlewareBase() = default;

MiddlewareCallContext::MiddlewareCallContext(impl::RpcData& data) : data_(data) {}

grpc::ClientContext& MiddlewareCallContext::GetContext() noexcept { return data_.GetContext(); }

std::string_view MiddlewareCallContext::GetClientName() const noexcept { return data_.GetClientName(); }

std::string_view MiddlewareCallContext::GetCallName() const noexcept { return data_.GetCallName(); }

CallKind MiddlewareCallContext::GetCallKind() const noexcept { return data_.GetCallKind(); }

tracing::Span& MiddlewareCallContext::GetSpan() noexcept { return data_.GetSpan(); }

impl::RpcData& MiddlewareCallContext::GetData(ugrpc::impl::InternalTag) { return data_; }

MiddlewareFactoryBase::~MiddlewareFactoryBase() = default;

namespace impl {

Middlewares InstantiateMiddlewares(const MiddlewareFactories& factories, const std::string& client_name) {
    Middlewares mws;
    mws.reserve(factories.size());
    for (const auto& mw_factory : factories) {
        mws.push_back(mw_factory->GetMiddleware(client_name));
    }
    return mws;
}

}  // namespace impl

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
