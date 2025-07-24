#pragma once

/// @file userver/ugrpc/client/call_context.hpp
/// @brief @copybrief ugrpc::client::CallContext

#include <grpcpp/client_context.h>

#include <userver/tracing/span.hpp>
#include <userver/utils/impl/internal_tag_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

namespace impl {
class CallState;
}  // namespace impl

/// @brief gRPC call context
class CallContext {
public:
    CallContext(utils::impl::InternalTag, impl::CallState& state);

    /// @returns the `ClientContext` used for this RPC
    grpc::ClientContext& GetClientContext() noexcept;

    /// @returns client name
    std::string_view GetClientName() const noexcept;

    /// @returns RPC name
    std::string_view GetCallName() const noexcept;

    /// @returns RPC span
    tracing::Span& GetSpan() noexcept;

private:
    impl::CallState& state_;
};

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
