#pragma once

#include <google/protobuf/message.h>
#include <grpcpp/server_context.h>

#include <userver/engine/single_waiting_task_mutex.hpp>
#include <userver/tracing/span.hpp>
#include <userver/ugrpc/impl/internal_tag_fwd.hpp>
#include <userver/ugrpc/impl/statistics_scope.hpp>
#include <userver/ugrpc/server/impl/call_kind.hpp>
#include <userver/ugrpc/server/impl/call_params.hpp>
#include <userver/ugrpc/server/middlewares/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

/// @brief A non-typed base class for any gRPC call
class CallAnyBase {
public:
    /// @returns the `ServerContext` used for this RPC
    /// @note Initial server metadata is not currently supported
    /// @note Trailing metadata, if any, must be set before the `Finish` call
    grpc::ServerContext& GetContext() { return params_.context; }

    /// @brief Name of the RPC in the format `full.path.ServiceName/MethodName`
    std::string_view GetCallName() const { return params_.call_name; }

    /// @brief Get name of gRPC service
    std::string_view GetServiceName() const;

    /// @brief Get name of called gRPC method
    std::string_view GetMethodName() const;

    /// @brief Get the span of the current RPC. Span's lifetime covers the
    /// `Handle` call of the outermost @ref MiddlewareBase "middleware".
    tracing::Span& GetSpan() { return params_.call_span; }

    /// @brief Get RPCs kind of method
    impl::CallKind GetCallKind() const { return call_kind_; }

    /// @brief Returns call context for storing per-call custom data
    ///
    /// The context can be used to pass data from server middleware to client
    /// handler or from one middleware to another one.
    ///
    /// ## Example usage:
    ///
    /// In authentication middleware:
    ///
    /// @code
    /// if (password_is_correct) {
    ///   // Username is authenticated, set it in per-call storage context
    ///   ctx.GetCall().GetStorageContext().Emplace(kAuthUsername, username);
    /// }
    /// @endcode
    ///
    /// In client handler:
    ///
    /// @code
    /// const auto& username = rpc.GetStorageContext().Get(kAuthUsername);
    /// auto msg = fmt::format("Hello, {}!", username);
    /// @endcode
    utils::AnyStorage<StorageContext>& GetStorageContext() { return params_.storage_context; }

    /// @brief Set a custom call name for metric labels
    void SetMetricsCallName(std::string_view call_name);

    /// @cond
    // For internal use only
    CallAnyBase(utils::impl::InternalTag, impl::CallParams&& params, impl::CallKind call_kind)
        : params_(std::move(params)), call_kind_(call_kind) {}

    // For internal use only
    ugrpc::impl::RpcStatisticsScope& GetStatistics(ugrpc::impl::InternalTag);

    // For internal use only
    void SetMiddlewareCallContext(utils::impl::InternalTag, MiddlewareCallContext& md_call_context) {
        middleware_call_context_ = &md_call_context;
    }
    /// @endcond

protected:
    ugrpc::impl::RpcStatisticsScope& GetStatistics() { return params_.statistics; }

    void ApplyRequestHook(google::protobuf::Message* request);

    void ApplyResponseHook(google::protobuf::Message* response);

    void PreSendStatus(const grpc::Status& status) noexcept;

    void PostFinish(const grpc::Status& status) noexcept;

private:
    std::unique_lock<engine::SingleWaitingTaskMutex> TakeMutexIfBidirectional();

    impl::CallParams params_;
    impl::CallKind call_kind_;
    MiddlewareCallContext* middleware_call_context_{nullptr};
    engine::SingleWaitingTaskMutex mutex_;
};

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
