#include <userver/ugrpc/client/middlewares/base.hpp>

#include <userver/utils/impl/internal_tag.hpp>

#include <userver/ugrpc/client/impl/call_state.hpp>

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

MiddlewareCallContext::MiddlewareCallContext(impl::CallState& state) : state_(state) {}

grpc::ClientContext& MiddlewareCallContext::GetClientContext() noexcept { return state_.GetClientContext(); }

std::string_view MiddlewareCallContext::GetClientName() const noexcept { return state_.GetClientName(); }

std::string_view MiddlewareCallContext::GetCallName() const noexcept { return state_.GetCallName(); }

tracing::Span& MiddlewareCallContext::GetSpan() noexcept { return state_.GetSpan(); }

bool MiddlewareCallContext::IsClientStreaming() const noexcept { return impl::IsClientStreaming(state_.GetCallKind()); }

bool MiddlewareCallContext::IsServerStreaming() const noexcept { return impl::IsServerStreaming(state_.GetCallKind()); }

impl::CallState& MiddlewareCallContext::GetState(utils::impl::InternalTag) { return state_; }

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
