#pragma once

/// @file userver/storages/sqlite/operation_types.hpp
/// @brief Operation properties for SQLite client

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

/// @brief Type of the SQL operation (read or write).
///
/// This enun is used to explicitly specify the expected type of query (read-only or read-write).
/// The value affects the choice of the connection pool: read-only operations are routed to a read-only pool,
/// while write-capable operations are executed in a read-write pool.
/// This helps to optimize resource usage and performance depending on the nature of the SQL request.
enum class OperationType {
    /// @brief Execute the query using a read-only connection pool.
    kReadOnly,
    /// @brief Execute the query using a read-write connection pool.
    kReadWrite,
};

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
