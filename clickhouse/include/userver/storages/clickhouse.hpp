#pragma once

/// @file userver/storages/clickhouse.hpp
/// This file is mainly for documentation purposes and inclusion of all headers
/// that are required for working with ClickHouse µserver component.

#include <userver/storages/clickhouse/cluster.hpp>
#include <userver/storages/clickhouse/component.hpp>
#include <userver/storages/clickhouse/execution_result.hpp>
#include <userver/storages/clickhouse/options.hpp>
#include <userver/storages/clickhouse/query.hpp>

USERVER_NAMESPACE_BEGIN

/// @brief Top namespace for uClickHouse driver.
///
/// For more information see @ref scripts/docs/en/userver/clickhouse/driver.md.
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
