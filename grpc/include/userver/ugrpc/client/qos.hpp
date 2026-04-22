#pragma once

/// @file userver/ugrpc/client/qos.hpp
/// @brief @copybrief ugrpc::client::Qos

#include <chrono>
#include <optional>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

/// @brief Per-RPC quality-of-service config. Taken from
/// @ref ugrpc::client::ClientQos. Can also be passed to ugrpc client methods
/// manually.
struct Qos final {
    /// @brief The maximum number of RPC attempts, including the original attempt.
    /// If set must be minimum 1.
    /// If `std::nullopt`, default grpc++ retry configuration is used.
    ///
    /// See also [the official gRPC docs on retries](https://grpc.io/docs/guides/retry/).
    std::optional<int> attempts;

    /// @brief Timeout for a single RPC attempt.
    /// If `std::nullopt`, no static timeout is applied, which is reasonable
    /// for streaming RPCs.
    ///
    /// @ref scripts/docs/en/userver/deadline_propagation.md "Deadline propagation",
    /// when enabled, uses the minimum of the remaining deadline time and this timeout
    /// as the actual timeout for the RPC attempt.
    ///
    /// @note The problem of "dead servers" is typically solved using
    /// [keepalive pings](https://github.com/grpc/grpc/blob/master/doc/keepalive.md),
    /// not using timeouts.
    std::optional<std::chrono::milliseconds> timeout;
};

bool operator==(const Qos& lhs, const Qos& rhs) noexcept;

Qos Parse(const formats::json::Value& value, formats::parse::To<Qos>);

formats::json::Value Serialize(const Qos& qos, formats::serialize::To<formats::json::Value>);

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
