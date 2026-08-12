#pragma once

/// @file userver/ugrpc/client/retry_limiter.hpp
/// @brief @copybrief ugrpc::client::RetryLimiter

#include <memory>

#include <grpcpp/support/status.h>

#include <userver/ugrpc/client/completion_status.hpp>
#include <userver/utils/expected.hpp>
#include <userver/utils/string_literal.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

/// @ingroup userver_base_classes
///
/// @brief Interface for controlling retry behavior based on request outcomes.
///
/// Tracks success/failure rates and decides whether retries should be allowed.
/// Implementations must be thread-safe.
///
/// @see RetryLimiterFactory
class RetryLimiter {
public:
    virtual ~RetryLimiter() = default;

    virtual void AccountCompletion(const CompletionStatus& completion_status) = 0;

    /// @brief Check if a retry should be attempted.
    /// @return true if retry is allowed, false otherwise.
    virtual bool CanRetry() const = 0;
};

/// @brief Settings for a @ref RetryLimiter instance.
///
/// @see @ref RetryLimiter
struct RetryLimiterSettings final {
    /// @brief Full call name of the gRPC method in format `service/method`.
    ///
    /// Example: `samples.api.GreeterService/SayHello`
    const utils::StringLiteral call_name;

    /// @brief Prefix for the full destination path in metrics
    ///
    /// @see @ref ugrpc::client::ClientSettings::destination_prefix_in_metrics
    std::string destination_prefix_in_metrics;
};

/// @ingroup userver_base_classes
///
/// @brief Abstract factory for creating @ref RetryLimiter instances.
///
/// Should be registered as a component and referenced via `retry-limiter` option in
/// static configuration of @ref ugrpc::client::CommonComponent.
///
/// @see @ref RetryLimiter
class RetryLimiterFactory {
public:
    virtual ~RetryLimiterFactory() = default;

    /// @brief Create a @ref RetryLimiter for a regular method.
    ///
    /// Called once per method during client initialization.
    /// The returned instance is reused for all requests to this method.
    ///
    /// @return @ref RetryLimiter instance or `nullptr` to disable throttling
    virtual std::unique_ptr<RetryLimiter> CreateRetryLimiter(RetryLimiterSettings&& settings) const = 0;
};

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
