#pragma once

/// @file userver/ugrpc/client/call_options.hpp
/// @brief @copybrief ugrpc::client::CallOptions

#include <chrono>
#include <string_view>
#include <utility>
#include <vector>

#include <grpcpp/client_context.h>
#include <grpcpp/support/config.h>

#include <userver/utils/move_only_function.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

namespace impl {
class CallOptionsAccessor;
}  // namespace impl

/// @brief Options passed to interface calls
class CallOptions {
public:
    /// @{
    /// Set and get retry attempts. Maximum number of retry attempts, including the original attempt.
    void SetAttempts(int attempts);
    int GetAttempts() const;
    /// @}

    /// @{
    /// Set and get operation timeout.
    ///
    /// In case of retries `timeout` applies to each attempt.
    /// Maximum time on call may actually be `timeout * attempts + sum(backoff_i)`
    void SetTimeout(std::chrono::milliseconds timeout);
    std::chrono::milliseconds GetTimeout() const;
    /// @}

    /// Add the (\a meta_key, \a meta_value) pair to the metadata associated with a client call.
    void AddMetadata(std::string_view meta_key, std::string_view meta_value);

    /// @{
    /// Set custom grpc::ClientContext factory.
    ///
    /// 'client_context_factory' may be called zero, once or more time, because of retries.
    using ClientContextFactory = utils::move_only_function<std::unique_ptr<grpc::ClientContext>() const>;
    void SetClientContextFactory(ClientContextFactory&& client_context_factory);
    /// @}

private:
    friend class impl::CallOptionsAccessor;

    int attempts_{0};

    std::chrono::milliseconds timeout_{std::chrono::milliseconds::max()};

    std::vector<std::pair<grpc::string, grpc::string>> metadata_;

    ClientContextFactory client_context_factory_;
};

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
