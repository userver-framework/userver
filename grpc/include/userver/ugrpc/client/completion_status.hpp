#pragma once

/// @file userver/ugrpc/client/completion_status.hpp
/// @brief @copybrief ugrpc::client::CompletionStatus

#include <cstdint>
#include <string_view>

#include <grpcpp/support/status.h>

#include <userver/utils/expected.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

/// @ingroup userver_clients
///
/// @brief Special completion types for gRPC client calls that don't result in a normal `grpc::Status`.
///
/// This enum represents exceptional completion scenarios that occur during gRPC client calls
/// when the RPC doesn't complete with a standard gRPC status. These cases are distinguished
/// from normal status codes to provide more precise information about the nature of the failure.
///
/// @see @ref CompletionStatus
enum class SpecialCaseCompletionType : std::uint8_t {
    /// [special_cases_declaration]
    /// @brief The RPC was interrupted at the network/transport level without a gRPC status.
    ///
    /// The call did not finish with a gRPC status from the server: the connection was lost, reset
    /// or otherwise interrupted. This is distinct from `grpc::StatusCode::UNAVAILABLE`, which is a
    /// valid gRPC status.
    ///
    /// For a unary call this is reported only when no concrete gRPC status is available and the
    /// response could not be obtained (not received, or received but not parseable). Whenever a
    /// concrete status is available instead (for example `UNAVAILABLE` or `CANCELLED`), it is
    /// reported as that `grpc::Status` rather than as this special case.
    ///
    /// Surfaces to the caller as @ref ugrpc::client::RpcInterruptedError; for unary calls it is
    /// retryable.
    kNetworkError,
    /// @brief Request timed out with deadline propagation enabled.
    ///
    /// This is distinguished from a regular `grpc::StatusCode::DEADLINE_EXCEEDED` to
    /// indicate that the timeout was due to propagated deadline from upstream request.
    kTimeoutDeadlinePropagated,
    /// @brief RPC was explicitly cancelled by the client.
    kCancelled,
    /// @brief RPC was abandoned without proper completion.
    ///
    /// The RPC object was destroyed before the call could complete normally.
    /// This happens when:
    /// - A streaming RPC object goes out of scope before `Finish()` is called
    /// - An async call future is destroyed without calling `Get()`
    /// - The client abandons the RPC without waiting for completion
    kAbandoned
    /// [special_cases_declaration]
};

/// @brief Convert SpecialCaseCompletionType to string representation.
std::string_view ToString(SpecialCaseCompletionType type);

/// @brief Convert SpecialCaseCompletionType to human-readable description.
std::string_view GetSpecialCaseCompletionTypeDescription(SpecialCaseCompletionType type);

/// @ingroup userver_clients
///
/// @brief Result type for gRPC client call completion.
///
/// Type alias for `CompletionStatus`.
/// Represents either a normal gRPC status (value) or a special completion case (error).
///
/// @see @ref SpecialCaseCompletionType
/// @see @ref ugrpc::client::MiddlewareBase::PostFinish
using CompletionStatus = utils::expected<grpc::Status, SpecialCaseCompletionType>;

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
