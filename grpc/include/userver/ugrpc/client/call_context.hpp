#pragma once

/// @file userver/ugrpc/client/call_context.hpp
/// @brief @copybrief ugrpc::client::CallContext

#include <grpcpp/client_context.h>

#include <userver/tracing/span.hpp>
#include <userver/utils/impl/internal_tag_fwd.hpp>
#include <userver/utils/move_only_function.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

namespace impl {
class CallState;
}  // namespace impl

/// @brief gRPC call context
class CallContext {
public:
    /// @cond
    // For internal use only
    CallContext(utils::impl::InternalTag, impl::CallState& state) noexcept;
    /// @endcond

    /// @returns the `ClientContext` used for this RPC
    grpc::ClientContext& GetClientContext() noexcept;

    /// @returns client name
    std::string_view GetClientName() const noexcept;

    /// @returns RPC name
    std::string_view GetCallName() const noexcept;

    /// @returns RPC span
    tracing::Span& GetSpan() noexcept;

    /// @cond
    // For internal use only
    impl::CallState& GetState(utils::impl::InternalTag) noexcept;
    /// @endcond

private:
    impl::CallState& state_;
};

class CancellableCallContext : public CallContext {
public:
    /// @cond
    // For internal use only
    using CancelFunction = utils::move_only_function<void()>;
    CancellableCallContext(utils::impl::InternalTag, impl::CallState& state, CancelFunction cancel_func) noexcept;
    /// @endcond

    /// Cancel associated call
    ///
    /// Can be called multiple times, Call could be in any stage.
    void Cancel();

private:
    CancelFunction cancel_func_;
};

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
