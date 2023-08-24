#pragma once

/// @file userver/storages/clickhouse/options.hpp
/// @brief Options

#include <chrono>
#include <optional>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse {

/// A structure to control timeouts for ClickHouse queries
///
/// There is a single parameter, `execute`, which limits the overall time
/// the driver spends executing a query, which includes:
/// * connecting to ClickHouse server, if there are no connections available and
///   connection pool still has space for new connections;
/// * waiting for a connection to become idle if there are no idle connections
///   and connection pool already has reached its max size;
/// * executing the statement;
/// * waiting for all the results to arrive.
///
/// In case of a timeout the client gets an exception and the driver drops the
/// connection (no further reuse will take place),
/// otherwise the connection is returned to the pool.
struct CommandControl final {
  /// Overall timeout for a command being executed.
  std::chrono::milliseconds execute;

  explicit constexpr CommandControl(std::chrono::milliseconds execute)
      : execute{execute} {}
};

/// @brief storages::clickhouse::CommandControl that may not be set.
using OptionalCommandControl = std::optional<CommandControl>;

}  // namespace storages::clickhouse

USERVER_NAMESPACE_END
