#pragma once

/// @file userver/ugrpc/client/call.hpp
/// @brief @copybrief ugrpc::client::CallAnyBase

#include <memory>

#include <grpcpp/client_context.h>

#include <userver/tracing/span.hpp>

#include <userver/ugrpc/client/impl/call_kind.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

namespace impl {
struct CallParams;
class CallState;
}  // namespace impl

/// @brief Base class for any RPC
class CallAnyBase {
public:
    /// @returns the `ClientContext` used for this RPC
    grpc::ClientContext& GetContext() noexcept;

    /// @returns client name
    std::string_view GetClientName() const noexcept;

    /// @returns RPC name
    std::string_view GetCallName() const noexcept;

    /// @returns RPC span
    tracing::Span& GetSpan();

protected:
    /// @cond
    CallAnyBase(impl::CallParams&& params, impl::CallKind call_kind);

    // Protected to prevent slicing.
    CallAnyBase(CallAnyBase&& other) noexcept;
    CallAnyBase& operator=(CallAnyBase&& other) noexcept;

    // Prevent ownership via pointer to base.
    ~CallAnyBase();

    impl::CallState& GetState() noexcept;

    const impl::CallState& GetState() const noexcept;

    bool IsValid() const noexcept { return state_.get(); }
    /// @endcond

private:
    std::unique_ptr<impl::CallState> state_;
};

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
