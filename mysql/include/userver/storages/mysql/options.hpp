#pragma once

/// @file userver/storages/mysql/options.hpp

#include <chrono>
#include <optional>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql {

using TimeoutDuration = std::chrono::milliseconds;

/// A structure to control timeouts for MySQL queries
///
/// There is a single parameter, `execute`, which limits the overall time
/// the driver spends executing a query, which includes:
/// * connecting to MySQL server, if there are no connections available and
///   connection pool still has space for new connections;
/// * waiting for a connection to become idle if there are no idle connections
///   and connection pool already has reached its max size;
/// * preparing the statement, if it's absent in statements cache;
/// * executing the statement;
/// * fetching all the statement results, if any, and not in cursor mode.
///
/// In case of a timeout the client gets an exception and the driver drops the
/// connection (no further reuse will take place),
/// otherwise the connection is returned to the pool.
struct CommandControl final {
  /// Overall timeout for a statement being executed.
  TimeoutDuration execute{};
};

/// @brief storages::mysql::CommandControl that may not be set.
using OptionalCommandControl = std::optional<CommandControl>;

}  // namespace storages::mysql

USERVER_NAMESPACE_END
