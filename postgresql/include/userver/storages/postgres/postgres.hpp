#pragma once

/// @file userver/storages/postgres/postgres.hpp
/// This file is mainly for documentation purposes and inclusion of all headers
/// that are required for working with PostgreSQL userver component.

#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/storages/postgres/exceptions.hpp>
#include <userver/storages/postgres/result_set.hpp>
#include <userver/storages/postgres/transaction.hpp>

/// @page pg_driver uPg Driver
///
/// 🐙 **userver** provides access to PostgreSQL database servers via
/// components::Postgres. The uPg driver is asynchronous, it suspends
/// current coroutine for carrying out network I/O.
///
/// @section features Features
/// - PostgreSQL cluster topology discovery;
/// - Manual cluster sharding (access to shard clusters by index);
/// - Connection pooling;
/// - Queries are transparently converted to prepared statements to use less
///   network on next execution, give the database more optimization freedom,
///   avoid the need for parameters escaping as the latter are now send
///   separately from the query;
/// - Automatic PgaaS topology discovery;
/// - Selecting query target (master/slave);
/// - Connection failover;
/// - Transaction support;
/// - Variadic template query parameter passing;
/// - Query result extraction to C++ types;
/// - More effective binary protocol usage for communication rather than the
///   libpq's text protocol;
/// - Caching the low-level database (D)escribe responses to save about a half
///   of network bandwidth on select statements that return multiple columns
///   (compared to the libpq implementation);
/// - Portals for effective background cache updates;
/// - Queries pipelining to execute multiple queries in one network roundtrip
///   (for example `begin + set transaction timeout + insert` result in one
///   roundtrip);
/// - Ability to manually control network roundtrips via
///   storages::postgres::QueryQueue to gain maximum efficiency
///   in case of multiple unrelated select statements;
/// - Mapping PostgreSQL user types to C++ types;
/// - Transaction error injection via pytest_userver.sql.RegisteredTrx;
/// - LISTEN/NOTIFY support via storages::postgres::Cluster::Listen();
/// - @ref scripts/docs/en/userver/deadline_propagation.md .
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
///
/// ----------
///
/// @htmlonly <div class="bottom-nav"> @endhtmlonly
/// ⇦ @ref scripts/docs/en/userver/lru_cache.md | @ref pg_transactions ⇨
/// @htmlonly </div> @endhtmlonly

USERVER_NAMESPACE_BEGIN

namespace storages {
/// @brief Top namespace for uPg driver
///
/// For more information see @ref pg_driver
namespace postgres {
/// @brief uPg input-output.
///
/// Namespace containing classes and functions for defining datatype
/// input-output and specifying mapping between C++ and PostgreSQL types.
namespace io {
/// @brief uPg input-output traits.
///
/// Namespace with metafunctions detecting different type traits needed for
/// PostgreSQL input-output operations
namespace traits {}  // namespace traits
}  // namespace io
}  // namespace postgres
}  // namespace storages

USERVER_NAMESPACE_END
