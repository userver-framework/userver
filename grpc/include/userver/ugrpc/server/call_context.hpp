#pragma once

/// @file userver/ugrpc/server/call_context.hpp
/// @brief @copybrief ugrpc::server::CallContext

#include <grpcpp/server_context.h>

#include <userver/tracing/span.hpp>
#include <userver/utils/any_storage.hpp>

#include <userver/ugrpc/server/storage_context.hpp>
#include <userver/utils/impl/internal_tag_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

namespace impl {
struct CallState;
}  // namespace impl

class CallContextBase {
public:
    /// @cond
    // For internal use only.
    CallContextBase(utils::impl::InternalTag, impl::CallState& state);
    /// @endcond

    CallContextBase(CallContextBase&&) = delete;
    CallContextBase& operator=(CallContextBase&&) = delete;

    /// @returns the `ServerContext` used for this RPC
    grpc::ServerContext& GetServerContext();

    /// @brief Name of the RPC in the format `full.path.ServiceName/MethodName`
    std::string_view GetCallName() const;

    /// @brief Get name of gRPC service
    std::string_view GetServiceName() const;

    /// @brief Get name of called gRPC method
    std::string_view GetMethodName() const;

    /// @brief Get the span of the current RPC
    tracing::Span& GetSpan();

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
    ///   context.GetStorageContext().Emplace(kAuthUsername, username);
    /// }
    /// @endcode
    ///
    /// In client handler:
    ///
    /// @code
    /// const auto& username = context.GetStorageContext().Get(kAuthUsername);
    /// auto msg = fmt::format("Hello, {}!", username);
    /// @endcode
    utils::AnyStorage<StorageContext>& GetStorageContext();

protected:
    /// @cond
    // For internal use only.
    const impl::CallState& GetCallState(utils::impl::InternalTag) const { return state_; }

    // For internal use only.
    impl::CallState& GetCallState(utils::impl::InternalTag) { return state_; }

    // Prevent destruction via pointer to base.
    ~CallContextBase() = default;
    /// @endcond

private:
    impl::CallState& state_;
};

/// @brief gRPC call context
class CallContext final : public CallContextBase {
public:
    /// @cond
    using CallContextBase::CallContextBase;
    /// @endcond
};

/// @brief generic gRPC call context
class GenericCallContext final : public CallContextBase {
public:
    /// @cond
    using CallContextBase::CallContextBase;
    /// @endcond

    /// @brief Set a custom call name for metric labels
    void SetMetricsCallName(std::string_view call_name);
};

}  // namespace ugrpc::server

USERVER_NAMESPACE_END
