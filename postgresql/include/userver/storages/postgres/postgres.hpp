#pragma once

/// @file userver/storages/postgres/postgres.hpp
/// This file is mainly for documentation purposes and inclusion of all headers
/// that are required for work with PostgreSQL µserver component.

#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/storages/postgres/exceptions.hpp>
#include <userver/storages/postgres/result_set.hpp>
#include <userver/storages/postgres/transaction.hpp>

/// @page pg_driver µPg Driver
///
/// µserver provides access to PostgreSQL database servers via
/// components::Postgres. The µPg driver is asynchronous, it suspends
/// current coroutine for carrying out network I/O.
///
/// @section features Features
/// - PostgreSQL cluster topology discovery;
/// - Manual cluster sharding (access to shard clusters by index);
/// - Connection pooling;
/// - Automatic PgaaS topology discovery;
/// - Selecting query target (master/slave);
/// - Connection failover (TODO)
/// - Transaction support;
/// - Variadic template query parameter passing;
/// - Query result extraction to C++ types;
/// - Mapping PostgreSQL user types to C++ types.
///
/// @section toc More information
/// - For configuration see components::Postgres
/// - For cluster topology see storages::postgres::Cluster
/// - @ref pg_transactions
/// - @ref pg_run_queries
/// - @ref pg_process_results
/// - @ref pg_types
/// - @ref pg_user_types
/// - @ref pg_errors
/// - @ref pg_topology

namespace storages {
/// @brief Top namespace for µPg driver
///
/// For more information see @ref pg_driver
namespace postgres {
/// @brief µPg input-output.
///
/// Namespace containing classes and functions for defining datatype
/// input-output and specifying mapping between C++ and PostgreSQL types.
namespace io {
/// @brief µPg input-output traits.
///
/// Namespace with metafunctions detecting different type traits needed for
/// PostgreSQL input-output operations
namespace traits {}  // namespace traits
}  // namespace io
}  // namespace postgres
}  // namespace storages
