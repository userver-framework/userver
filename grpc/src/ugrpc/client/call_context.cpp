#include <userver/ugrpc/client/call_context.hpp>

#include <userver/utils/impl/internal_tag.hpp>

#include <userver/ugrpc/client/impl/call_state.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

CallContext::CallContext(utils::impl::InternalTag, impl::CallState& state) noexcept : state_(state) {}

grpc::ClientContext& CallContext::GetClientContext() noexcept { return state_.GetClientContextCommitted(); }

std::string_view CallContext::GetClientName() const noexcept { return state_.GetClientName(); }

std::string_view CallContext::GetCallName() const noexcept { return state_.GetCallName(); }

tracing::Span& CallContext::GetSpan() noexcept { return state_.GetSpan(); }

impl::CallState& CallContext::GetState(utils::impl::InternalTag) noexcept { return state_; }

CancellableCallContext::CancellableCallContext(
    utils::impl::InternalTag,
    impl::CallState& state,
    CancelFunction cancel_func
) noexcept
    : CallContext(utils::impl::InternalTag{}, state), cancel_func_(std::move(cancel_func)) {}

void CancellableCallContext::Cancel() {
    if (cancel_func_) {
        cancel_func_();
    }
}

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
