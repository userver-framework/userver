#include <userver/ugrpc/client/call.hpp>

#include <userver/ugrpc/client/impl/call_state.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

grpc::ClientContext& CallAnyBase::GetContext() noexcept { return GetState().GetContext(); }

std::string_view CallAnyBase::GetClientName() const noexcept { return GetState().GetClientName(); }

std::string_view CallAnyBase::GetCallName() const noexcept { return GetState().GetCallName(); }

tracing::Span& CallAnyBase::GetSpan() { return GetState().GetSpan(); }

CallAnyBase::CallAnyBase(impl::CallParams&& params, impl::CallKind call_kind)
    : state_(std::make_unique<impl::CallState>(std::move(params), call_kind)) {}

CallAnyBase::CallAnyBase(CallAnyBase&& other) noexcept = default;

CallAnyBase& CallAnyBase::operator=(CallAnyBase&& other) noexcept = default;

CallAnyBase::~CallAnyBase() = default;

impl::CallState& CallAnyBase::GetState() noexcept {
    UASSERT(state_);
    return *state_;
}

const impl::CallState& CallAnyBase::GetState() const noexcept {
    UASSERT(state_);
    return *state_;
}

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
