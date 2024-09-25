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
  // clang-format off
  /// @brief An upper bound on the deadline applied to the entire RPC.
  /// If `std::nullopt`, no static deadline is applied, which is reasonable
  /// for streaming RPCs.
  ///
  /// @ref scripts/docs/en/userver/deadline_propagation.md "Deadline propagation",
  /// when enabled, also puts an upper bound on the RPC deadline.
  ///
  /// @note The problem of "dead servers" is typically solved using
  /// [keepalive pings](https://github.com/grpc/grpc/blob/master/doc/keepalive.md),
  /// not using timeouts.
  // clang-format on
  std::optional<std::chrono::milliseconds> timeout;
};

bool operator==(const Qos& lhs, const Qos& rhs) noexcept;

Qos Parse(const formats::json::Value& value, formats::parse::To<Qos>);

formats::json::Value Serialize(const Qos& qos,
                               formats::serialize::To<formats::json::Value>);

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
