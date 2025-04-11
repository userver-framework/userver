#pragma once

/// @file userver/ugrpc/client/call.hpp
/// @brief @copybrief ugrpc::client::CallAnyBase

#include <memory>

#include <grpcpp/client_context.h>

#include <userver/tracing/span.hpp>

#include <userver/ugrpc/client/impl/async_methods.hpp>
#include <userver/ugrpc/client/impl/call_kind.hpp>
#include <userver/ugrpc/client/impl/call_params.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

/// @brief Base class for any RPC
class CallAnyBase {
protected:
    /// @cond
    CallAnyBase(impl::CallParams&& params, impl::CallKind call_kind)
        : data_(std::make_unique<impl::RpcData>(std::move(params), call_kind)) {}
    /// @endcond

public:
    /// @returns the `ClientContext` used for this RPC
    grpc::ClientContext& GetContext();

    /// @returns client name
    std::string_view GetClientName() const;

    /// @returns RPC name
    std::string_view GetCallName() const;

    /// @returns RPC span
    tracing::Span& GetSpan();

protected:
    impl::RpcData& GetData();
    const impl::RpcData& GetData() const;

private:
    std::unique_ptr<impl::RpcData> data_;
};

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
