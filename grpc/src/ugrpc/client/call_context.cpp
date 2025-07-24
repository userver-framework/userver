#include <userver/ugrpc/client/call_context.hpp>

#include <userver/utils/impl/internal_tag.hpp>

#include <userver/ugrpc/client/impl/call_state.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

CallContext::CallContext(utils::impl::InternalTag, impl::CallState& state) : state_(state) {}

grpc::ClientContext& CallContext::GetClientContext() noexcept { return state_.GetClientContext(); }

std::string_view CallContext::GetClientName() const noexcept { return state_.GetClientName(); }

std::string_view CallContext::GetCallName() const noexcept { return state_.GetCallName(); }

tracing::Span& CallContext::GetSpan() noexcept { return state_.GetSpan(); }

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
