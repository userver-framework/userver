#pragma once

/// @file userver/storages/clickhouse.hpp
/// This file is mainly for documentation purposes and inclusion of all headers
/// that are required for working with ClickHouse µserver component.

#include <userver/storages/clickhouse/cluster.hpp>
#include <userver/storages/clickhouse/component.hpp>
#include <userver/storages/clickhouse/execution_result.hpp>
#include <userver/storages/clickhouse/options.hpp>
#include <userver/storages/clickhouse/query.hpp>

/// @page clickhouse_driver ClickHouse Driver
///
/// 🐙 **userver** provides access to ClickHouse databases servers via
/// components::ClickHouse. The uClickHouse driver is asynchronous, it suspends
/// current coroutine for carrying out network I/O.
///
/// @section feature Features
/// - Connection pooling;
/// - Variadic template query parameter passing;
/// - Query result extraction to C++ types;
/// - Mapping C++ types to native ClickHouse types.
///
/// @section info More information
/// - For configuration see components::ClickHouse
/// - For cluster operations see storages::clickhouse::Cluster
/// - For mapping C++ types to Clickhouse types see @ref clickhouse_io
///
/// ----------
///
/// @htmlonly <div class="bottom-nav"> @endhtmlonly
/// ⇦ @ref md_en_userver_redis | @ref md_en_userver_development_stability ⇨
/// @htmlonly </div> @endhtmlonly

USERVER_NAMESPACE_BEGIN

/// @brief Top namespace for uClickHouse driver.
///
/// For more information see @ref clickhouse_driver.
namespace storages::clickhouse {}

/// @brief uClickHouse input-output.
///
/// Namespace containing classes and functions for defining datatype
/// input-output and specifying mapping between C++ and ClickHouse types.
namespace storages::clickhouse::io {}

/// @brief uClickHouse columns.
///
/// Namespace containing definitions of supported ClickHouse column types.
/// For more information see @ref userver_clickhouse_types
namespace storages::clickhouse::io::columns {}

USERVER_NAMESPACE_END
