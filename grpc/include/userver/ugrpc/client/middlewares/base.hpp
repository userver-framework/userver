#pragma once

/// @file userver/ugrpc/client/middlewares/base.hpp
/// @brief @copybrief ugrpc::client::MiddlewareBase

#include <optional>
#include <string_view>

#include <google/protobuf/message.h>
#include <grpcpp/client_context.h>
#include <grpcpp/support/status.h>

#include <userver/components/component_base.hpp>
#include <userver/middlewares/groups.hpp>
#include <userver/middlewares/runner.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/impl/internal_tag_fwd.hpp>

#include <userver/ugrpc/time_utils.hpp>

USERVER_NAMESPACE_BEGIN

/// @see @ref scripts/docs/en/userver/grpc/client_middlewares.md
namespace ugrpc::client::middlewares {}

namespace ugrpc::client {

namespace impl {
class CallState;
}  // namespace impl

/// @ingroup userver_grpc_client_middlewares
///
/// @brief Client meta info for a middleware construction.
struct ClientInfo final {
    /// The name that is passed to `ClientFactory::MakeClient`.
    std::string client_name{};
    /// `std::nullopt` for generic clients.
    std::optional<std::string> service_full_name{};
};

/// @ingroup userver_grpc_client_middlewares
///
/// @brief Context for middleware-specific data during gRPC call
///
/// It is created for each gRPC Call and it stores aux. data
/// used by middlewares. Each registered middleware is called by
/// `Middleware::Handle` with the context passed as an argument.
/// A middleware may access Call and initial request (if any) using the context.
class MiddlewareCallContext final {
public:
    /// @cond
    explicit MiddlewareCallContext(impl::CallState& state);
    /// @endcond

    /// @returns the `ClientContext` used for this RPC
    grpc::ClientContext& GetClientContext() noexcept;

    /// @returns client name
    std::string_view GetClientName() const noexcept;

    /// @returns RPC name
    std::string_view GetCallName() const noexcept;

    /// @returns RPC span
    tracing::Span& GetSpan() noexcept;

    /// @returns Is a client-side streaming call
    bool IsClientStreaming() const noexcept;

    /// @returns Is a server-side streaming call
    bool IsServerStreaming() const noexcept;

    /// @cond
    // For internal use only
    impl::CallState& GetState(utils::impl::InternalTag);
    /// @endcond

private:
    impl::CallState& state_;
};

/// @ingroup userver_base_classes userver_grpc_client_middlewares
///
/// @brief Base class for client gRPC middleware
class MiddlewareBase {
public:
    virtual ~MiddlewareBase();

    MiddlewareBase(const MiddlewareBase&) = delete;
    MiddlewareBase(MiddlewareBase&&) = delete;

    MiddlewareBase& operator=(const MiddlewareBase&) = delete;
    MiddlewareBase& operator=(MiddlewareBase&&) = delete;

    /// @brief This function is called before rpc, on each rpc. It does nothing by
    /// default
    virtual void PreStartCall(MiddlewareCallContext&) const;

    /// @brief This function is called before sending message, on each request. It
    /// does nothing by default
    /// @note  Not called for `GenericClient` messages
    virtual void PreSendMessage(MiddlewareCallContext&, const google::protobuf::Message&) const;

    /// @brief This function is called after receiving message, on each response.
    /// It does nothing by default
    /// @note  Not called for `GenericClient` messages
    virtual void PostRecvMessage(MiddlewareCallContext&, const google::protobuf::Message&) const;

    /// @brief This function is called after rpc, on each rpc. It does nothing by
    /// default
    /// @note Could be not called in case of deadline or network problem
    /// @see @ref ugrpc::client::RpcInterruptedError
    virtual void PostFinish(MiddlewareCallContext&, const grpc::Status&) const;

protected:
    MiddlewareBase();
};

/// @ingroup userver_base_classes
///
/// @brief Factory that creates specific client middlewares for clients.
///
/// Override ugrpc::client::SimpleMiddlewareFactoryComponent::CreateMiddleware to create middleware for your gRPC
/// client. If you declare a static config for a middleware, you must override
/// ugrpc::client::SimpleMiddlewareFactoryComponent::GetMiddlewareConfigSchema.
///
/// If you are not going to use static config, ugrpc::client::ClientInfo and your middleware is default constructible,
/// just use ugrpc::client::SimpleMiddlewareFactoryComponent.
///
/// ## Example:
///
/// @snippet samples/grpc_middleware_service/src/middlewares/client/chaos.hpp gRPC middleware sample
/// @snippet samples/grpc_middleware_service/src/middlewares/client/chaos.cpp gRPC middleware sample

using MiddlewareFactoryComponentBase =
    USERVER_NAMESPACE::middlewares::MiddlewareFactoryComponentBase<MiddlewareBase, ClientInfo>;

/// @ingroup userver_components
///
/// @brief The alias for a short-cut client factory.
///
/// ## Static options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// enabled | the flag to enable/disable middleware in the pipeline | true
///
/// ## Example usage:
///
/// @snippet samples/grpc_middleware_service/src/middlewares/client/auth.hpp Middleware declaration
///
/// ## Static config example
///
/// @snippet samples/grpc_middleware_service/configs/static_config.yaml static config grpc-auth-client

template <typename Middleware>
using SimpleMiddlewareFactoryComponent =
    USERVER_NAMESPACE::middlewares::impl::SimpleMiddlewareFactoryComponent<MiddlewareBase, Middleware, ClientInfo>;

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
