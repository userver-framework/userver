#pragma once

/// @file userver/ugrpc/client/call_context.hpp
/// @brief @copybrief ugrpc::client::CallContext

#include <grpcpp/client_context.h>

#include <userver/utils/impl/internal_tag_fwd.hpp>

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
    grpc::ClientContext& GetClientContext();

    /// @returns client name
    std::string_view GetClientName() const noexcept;

    /// @returns RPC name
    std::string_view GetCallName() const noexcept;

private:
    impl::CallState& state_;
};

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
