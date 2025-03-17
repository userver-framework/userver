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

MiddlewarePipelineComponent::MiddlewarePipelineComponent(
    const components::ComponentConfig& config,
    const components::ComponentContext& context
)
    : USERVER_NAMESPACE::middlewares::impl::AnyMiddlewarePipelineComponent(
          config,
          context,
          {{
              {"grpc-client-logging", {}},
              {"grpc-client-baggage", {}},
              {"grpc-client-deadline-propagation", {}},
          }}
      ) {}

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
