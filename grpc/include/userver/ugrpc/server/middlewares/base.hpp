#pragma once

/// @file userver/ugrpc/server/middlewares/base.hpp
/// @brief @copybrief ugrpc::server::MiddlewareBase

#include <string>

#include <google/protobuf/message.h>
#include <grpcpp/support/status.h>

#include <userver/components/component_base.hpp>
#include <userver/dynamic_config/snapshot.hpp>
#include <userver/middlewares/groups.hpp>
#include <userver/middlewares/runner.hpp>
#include <userver/utils/impl/internal_tag_fwd.hpp>

#include <userver/ugrpc/server/call_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

class RpcStatisticsScope;

}  // namespace ugrpc::impl

/// @see @ref scripts/docs/en/userver/grpc/server_middlewares.md
namespace ugrpc::server::middlewares {}

namespace ugrpc::server {

/// @ingroup userver_grpc_server_middlewares
///
/// @brief Service meta info for a middleware construction.
struct ServiceInfo final {
    std::string full_service_name{};
};

/// @ingroup userver_grpc_server_middlewares
///
/// @brief Context for middleware-specific data during gRPC call.
class MiddlewareCallContext final : public CallContextBase {
public:
    /// @cond
    // For internal use only
    MiddlewareCallContext(utils::impl::InternalTag, impl::CallState& state);
    /// @endcond

    /// @brief Aborts the RPC, returning the specified status to the upstream client, see details below.
    ///
    /// It should be the last command in middlewares hooks.
    ///
    /// If that method is called in methods:
    ///
    /// 1. @ref MiddlewareBase::OnCallStart - remaining OnCallStart hooks won't be called. Will be called OnCallFinish
    ///    hooks of middlewares that was called before `SetError`
    /// 2. @ref MiddlewareBase::PostRecvMessage or @ref ugrpc::server::MiddlewareBase::PreSendMessage :
    ///    * unary: handler won't be called - all. All `OnCallFinish` hooks will be called.
    ///    * stream: from Read/Write throws a special exception, that ends a handler. All `OnCallFinish` hooks will be
    ///      called.
    /// 3. @ref MiddlewareBase::OnCallFinish - all `OnCallFinish` will be called, despite of `SetError` and exceptions.
    ///    If the request is going to end with the error status, then the status is replaced with the status of the
    ///    current hook.
    ///
    /// Example usage
    ///
    /// @snippet samples/grpc_middleware_service/src/middlewares/server/auth.cpp Middleware implementation
    void SetError(grpc::Status&& status) noexcept;

    /// @returns Is a client-side streaming call
    bool IsClientStreaming() const noexcept;

    /// @returns Is a server-side streaming call
    bool IsServerStreaming() const noexcept;

    /// @brief Get values extracted from dynamic_config. Snapshot will be
    /// deleted when the last middleware completes.
    const dynamic_config::Snapshot& GetInitialDynamicConfig() const;

    /// @cond
    // For internal use only.
    ugrpc::impl::RpcStatisticsScope& GetStatistics(utils::impl::InternalTag);

    // For internal use only.
    grpc::Status& GetStatus(utils::impl::InternalTag) { return status_; }
    /// @endcond

private:
    grpc::Status status_;
};

/// @ingroup userver_base_classes userver_grpc_server_middlewares
///
/// @brief Base class for server gRPC middleware
class MiddlewareBase {
public:
    MiddlewareBase();
    MiddlewareBase(const MiddlewareBase&) = delete;
    MiddlewareBase& operator=(const MiddlewareBase&) = delete;
    MiddlewareBase& operator=(MiddlewareBase&&) = delete;

    virtual ~MiddlewareBase();

    /// @brief This hook is invoked once per Call (RPC), after the message metadata is received, but before the handler
    /// function is called.
    ///
    /// If all OnCallStart succeeded => OnCallFinish will invoked after a success method call.
    virtual void OnCallStart(MiddlewareCallContext& context) const;

    /// @brief The function is invoked after each received message.
    ///
    /// PostRecvMessage is called:
    ///  * unary: exactly once per Call (RPC)
    ///  * stream: 0, 1 or more times
    virtual void PostRecvMessage(MiddlewareCallContext& context, google::protobuf::Message& request) const;

    /// @brief The function is invoked before each sended message.
    ///
    /// PreSendMessage is called:
    ///  * unary: 0 or 1 per Call (RPC), depending on whether the RPC returns a response or a failed status
    ///  * stream: once per response message, that is, 0, 1, more times per Call (RPC).
    virtual void PreSendMessage(MiddlewareCallContext& context, google::protobuf::Message& response) const;

    /// @brief This hook is invoked once per Call (RPC), after the handler function returns, but before the message is
    /// sent to the upstream client.
    ///
    /// All OnCallStart invoked in the reverse order relatively OnCallFinish. You can change grpc status and it will
    /// apply for a rpc call.
    ///
    /// @warning If Handler (grpc method) returns !ok status, OnCallFinish won't be called.
    virtual void OnCallFinish(MiddlewareCallContext& context, const grpc::Status& status) const;
};

/// @ingroup userver_base_classes
///
/// @brief Factory that creates specific server middlewares for services.
///
/// Override `CreateMiddleware` to create middleware for your gRPC service.
/// If you declare a static config for a middleware, you must override `GetMiddlewareConfigSchema`.
///
/// @note If you are not going to use a static config, ugrpc::server::ServiceInfo and your middleware is default
/// constructible, just use ugrpc::server::SimpleMiddlewareFactoryComponent.
///
/// ## Example:
///
/// @snippet samples/grpc_middleware_service/src/middlewares/server/meta_filter.hpp gRPC middleware sample
/// @snippet samples/grpc_middleware_service/src/middlewares/server/meta_filter.cpp gRPC middleware sample
///
/// And there is a possibility to override the middleware config per service:
///
/// @snippet samples/grpc_middleware_service/configs/static_config.yaml gRPC greeter-service sample

using MiddlewareFactoryComponentBase =
    USERVER_NAMESPACE::middlewares::MiddlewareFactoryComponentBase<MiddlewareBase, ServiceInfo>;

/// @ingroup userver_components
///
/// @brief The alias for a short-cut server middleware factory.
///
/// ## Static options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// enabled | the flag to enable/disable middleware in the pipeline | true
///
/// ## Example usage:
///
/// @snippet samples/grpc_middleware_service/src/middlewares/server/auth.hpp Middleware declaration
///
/// ## Static config example
///
/// @snippet samples/grpc_middleware_service/configs/static_config.yaml grpc-server-auth static config

template <typename Middleware>
using SimpleMiddlewareFactoryComponent =
    USERVER_NAMESPACE::middlewares::impl::SimpleMiddlewareFactoryComponent<MiddlewareBase, Middleware, ServiceInfo>;

}  // namespace ugrpc::server

USERVER_NAMESPACE_END
